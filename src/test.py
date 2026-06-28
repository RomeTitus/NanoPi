from smbus import SMBus
import time

I2C_ADDR = 0x04
BUS_NUM = 1

DEFAULT_RETRIES = 3
RETRY_DELAY = 0.15
I2C_POLL_INTERVAL = 0.005
I2C_AFTER_WRITE_DELAY = 0.001
I2C_BUSY_RETRY_DELAY = 0.02
ULTRASONIC_VALUE_RETRIES = 2
MAX_I2C_BYTES = 32
MAX_RESPONSE_RECORDS = 7

STATUS_OK = 0x00
ERR_ULTRASONIC_TIMEOUT = 0xFB
ERR_BUSY = 0xFA
ERR_FRAMING = 0xFC
ERR_UNKNOWN_COMMAND = 0xFD
ERR_VALIDATION = 0xFE

STATUS_NAMES = {
    STATUS_OK: "OK",
    ERR_ULTRASONIC_TIMEOUT: "ULTRASONIC_TIMEOUT",
    ERR_BUSY: "BUSY",
    ERR_FRAMING: "FRAMING_ERROR",
    ERR_UNKNOWN_COMMAND: "UNKNOWN_COMMAND",
    ERR_VALIDATION: "VALIDATION_ERROR",
}


class _I2CTransport:
    """
    Private class that manages low-level I2C communication, retry logic, 
    and batch command splitting.
    """
    
    def __init__(self, bus_num=BUS_NUM, addr=I2C_ADDR):
        self.addr = addr
        self.bus = None

        try:
            self.bus = SMBus(bus_num)
        except Exception as e:
            print(f"[I2C] Bus init failed: {e}")
            self.bus = None

    def is_available(self):
        """Check if device responds on I2C"""
        if self.bus is None:
            return False

        try:
            # Quick probe (no data write)
            self.bus.write_quick(self.addr)
            return True
        except Exception:
            return False

    def _transfer(self, write_data, read_len, retries=DEFAULT_RETRIES, timeout=2.0):
        """
        Smart I2C transfer:
        - Write once
        - Poll for valid response
        - Stop on success or timeout
        """

        if self.bus is None:
            raise RuntimeError("I2C bus not initialized")

        last_error = None

        for _ in range(retries):
            try:
                # ================= WRITE ONCE =================
                if write_data:
                    cmd = write_data[0]
                    payload = write_data[1:]
                    self.bus.write_i2c_block_data(self.addr, cmd, payload)
                    # Give the slave ISR/process loop a brief moment before first read.
                    time.sleep(I2C_AFTER_WRITE_DELAY)

                start_time = time.time()

                # ================= POLL =================
                while (time.time() - start_time) < timeout:
                    try:
                        if read_len > 0:
                            data = self.bus.read_i2c_block_data(self.addr, 0, read_len)
                        else:
                            return None

                        # Busy frames are explicit now: [1][BUSY][0][0][0][checksum]. Keep polling.
                        if data and len(data) > 1 and data[0] == 1 and data[1] == ERR_BUSY:
                            time.sleep(I2C_BUSY_RETRY_DELAY)
                            continue

                        # Valid ready response has at least one record (count 1..7).
                        if data and len(data) > 1 and 1 <= data[0] <= MAX_RESPONSE_RECORDS:
                            return data

                        # ⚠️ Not ready yet → keep polling
                        time.sleep(I2C_POLL_INTERVAL)

                    except Exception as e:
                        # Read failed → retry within timeout window
                        last_error = e
                        time.sleep(I2C_POLL_INTERVAL)

                # ⏱ Timeout → retry full transaction
                time.sleep(RETRY_DELAY)

            except Exception as e:
                last_error = e
                time.sleep(RETRY_DELAY)

        raise RuntimeError(f"I2C transfer timeout after retries: {last_error}")

    @staticmethod
    def _xor_checksum(data):
        checksum = 0
        for b in data:
            checksum ^= (b & 0xFF)
        return checksum & 0xFF

    def _decode_response_frame(self, raw_response):
        """
        Decode transport frame: [count][records...][checksum]
        Each record is 4 bytes: [status][value_hi][value_mid][value_lo]
        """
        if not raw_response or len(raw_response) < 2:
            raise RuntimeError("Response too short to contain checksum")

        count = raw_response[0]
        if count > MAX_RESPONSE_RECORDS:
            raise RuntimeError(f"Invalid record count in frame: {count}")

        expected_len = 1 + (count * 4) + 1
        if len(raw_response) < expected_len:
            raise RuntimeError(
                f"Short frame: expected {expected_len} bytes, got {len(raw_response)}"
            )

        frame = raw_response[:expected_len]
        payload = frame[:-1]
        received_checksum = frame[-1]
        calculated_checksum = self._xor_checksum(payload)

        if calculated_checksum != received_checksum:
            raise RuntimeError(
                f"Checksum mismatch: calc=0x{calculated_checksum:02X}, recv=0x{received_checksum:02X}, frame={frame}"
            )

        records = []
        offset = 1
        for _ in range(count):
            records.append(frame[offset:offset + 4])
            offset += 4

        return records

    def _parse_response_record(self, record):
        """
        Parse one fixed-size response record.
        Record format: [status][value_hi][value_mid][value_lo]
        """
        if len(record) != 4:
            raise RuntimeError(f"Invalid response record size: {record}")

        status = record[0]
        value = (record[1] << 16) | (record[2] << 8) | record[3]

        if status == STATUS_OK:
            return {
                "status": status,
                "value": value
            }

        # Ultrasonic timeout is expected occasionally in noisy conditions.
        # Return it as non-fatal so batch reads still complete.
        if status == ERR_ULTRASONIC_TIMEOUT:
            return {
                "status": status,
                "value": None
            }

        if status == ERR_BUSY:
            return {
                "status": status,
                "value": None
            }

        if status != STATUS_OK:
            status_name = STATUS_NAMES.get(status, "UNKNOWN_STATUS")
            raise RuntimeError(
                f"Device returned error status 0x{status:02X} ({status_name}), payload={record[1:]}"
            )

    def send_batch(self, commands, retries=DEFAULT_RETRIES, flags=0x00):
        """
        Send up to 8 binary commands in a single batch.
        Due to SMBus 32-byte limit, batches are automatically split if needed.
        Response frame is: [count][records...][checksum], where each record is 4 bytes.
        Max 7 response records fit in one transfer (1 + 7*4 + 1 = 30 bytes).
        
        Args:
            commands: List of command byte arrays (each starting with 0xAA, max 10 bytes)
            retries: Number of retry attempts
            
        Returns:
            List of parsed response dicts, one per command
            
        Raises:
            RuntimeError: If I2C transfer fails
        """
        if len(commands) == 0:
            raise ValueError("At least one command required")
        
        # Validate all commands
        for i, cmd in enumerate(commands):
            if len(cmd) > 10:
                raise ValueError(f"Command {i} too long: {len(cmd)} > 10 bytes")
            if cmd[0] != 0xAA:
                raise ValueError(f"Command {i} must start with 0xAA")
        
        # Split commands into batches that fit within 32-byte SMBus limit.
        # Request frame includes 1-byte flags prefix.
        # Response frame supports up to 7 records per transfer.
        all_responses = []
        batch_start = 0
        
        while batch_start < len(commands):
            # Determine how many commands fit in this batch
            batch_size = 0
            batch_bytes = 1  # flags byte in request frame
            max_commands_by_response = 7
            
            for i in range(batch_start, min(batch_start + max_commands_by_response, len(commands))):
                cmd_bytes = len(commands[i])
                
                # Check if adding this command would exceed 32-byte request frame.
                if batch_bytes + cmd_bytes > MAX_I2C_BYTES:
                    break
                
                batch_size += 1
                batch_bytes += cmd_bytes
            
            if batch_size == 0:
                raise ValueError(f"Command {batch_start} exceeds 32-byte limits")
            
            # Send this batch
            batch_commands = commands[batch_start:batch_start + batch_size]
            batch_responses = self._send_batch_internal(batch_commands, retries, flags)
            all_responses.extend(batch_responses)
            
            batch_start += batch_size
        
        return all_responses
    
    def _send_batch_internal(self, commands, retries=DEFAULT_RETRIES, flags=0x00):
        """
        Internal method to send a single batch without splitting.
        Assumes commands already fit within 32-byte limits.
        """
        # Build request frame: [flags][payload...]
        batch_data = [flags & 0xFF]
        for cmd in commands:
            batch_data.extend(cmd)

        if len(batch_data) > MAX_I2C_BYTES:
            raise ValueError(
                f"Request too long for I2C frame: {len(batch_data)} > {MAX_I2C_BYTES}"
            )
        
        # Read full frame budget to handle variable response length.
        max_read_len = MAX_I2C_BYTES
        
        # Send batch and get all responses
        raw_response = self._transfer(batch_data, max_read_len, retries)
        records = self._decode_response_frame(raw_response)

        responses = []
        for record in records:
            try:
                parsed = self._parse_response_record(record)
                responses.append(parsed)
            except RuntimeError as e:
                raise RuntimeError(f"Failed to parse response: {e}")

        if len(responses) != len(commands):
            raise RuntimeError(
                f"Response count mismatch: expected {len(commands)}, got {len(responses)}"
            )
        
        return responses


class NanoI2C:
    """
    Public class for high-level Nano I2C operations.
    Delegates low-level communication to _I2CTransport.
    """
    
    def __init__(self, bus_num=BUS_NUM, addr=I2C_ADDR):
        self._transport = _I2CTransport(bus_num, addr)

    def is_available(self):
        """Check if device responds on I2C"""
        return self._transport.is_available()

    # ================= TEXT =================
    def _build_text_command(self, text):
        """Build a text command (no I2C communication)."""
        return [ord(c) for c in text]

    # ================= BATCH COMMAND SENDER =================
    def send_batch(self, commands, retries=DEFAULT_RETRIES, flags=0x00):
        """Delegate to transport layer."""
        return self._transport.send_batch(commands, retries, flags)

    def send_text(self, text, retries=DEFAULT_RETRIES):
        """Send a text command and return the response as a string."""
        cmd = self._build_text_command(text)
        # Pad to read full response
        resp = self._transport._transfer(cmd, 32, retries)
        return ''.join(chr(b) for b in resp if b != 0).strip()

    # ================= COMMAND BUILDERS =================
    # Each builder is responsible for creating command bytes only (no I2C communication)
    
    def _build_read_command(self, pin):
        """Build command to read pin."""
        return [0xAA, 0x01, 1, pin]
    
    def _build_write_command(self, pin, value):
        """Build command to write pin."""
        return [0xAA, 0x01, 0, pin, 1 if value else 0]
    
    def _build_ultrasonic_read_command(self, trigger, echo):
        """Build command to read ultrasonic sensor."""
        return [0xAA, 0x02, trigger, echo]

    # ================= BATCH OPERATIONS ================="
    # Methods for executing multiple commands at once
    
    def read_multiple(self, pins, debug=False):
        """
        Read multiple digital pins in a batch.
        Automatically handles splitting for SMBus limits.
        
        Args:
            pins: List of pin numbers
            
        Returns:
            List of pin values
        """
        commands = [self._build_read_command(pin) for pin in pins]
        flags = 0x01 if debug else 0x00
        responses = self.send_batch(commands, flags=flags)
        return [resp["value"] & 0xFFFF for resp in responses]
    
    def write_multiple(self, pin_values, debug=False):
        """
        Write to multiple digital pins in one batch.
        Automatically handles splitting for SMBus limits.
        
        Args:
            pin_values: List of (pin, value) tuples
            
        Returns:
            List of responses
        """
        commands = [self._build_write_command(pin, val) for pin, val in pin_values]
        flags = 0x01 if debug else 0x00
        return self.send_batch(commands, flags=flags)
    
    def ultrasonic_read_multiple(self, sensor_pairs, debug=False):
        """
        Read multiple ultrasonic sensors in one batch.
        Automatically handles splitting for SMBus limits.
        
        Args:
            sensor_pairs: List of (trigger_pin, echo_pin) tuples
            
        Returns:
            List of distance values
        """
        commands = [self._build_ultrasonic_read_command(trigger, echo) for trigger, echo in sensor_pairs]
        flags = 0x01 if debug else 0x00
        responses = self.send_batch(commands, flags=flags)
        values = []
        for index, resp in enumerate(responses):
            value = resp["value"]

            if value is None:
                # Retry a timeout locally before giving up on this measurement.
                retry_value = None
                for _ in range(ULTRASONIC_VALUE_RETRIES):
                    time.sleep(0.06)
                    retry_responses = self.send_batch([commands[index]], flags=flags)
                    retry_value = retry_responses[0]["value"]
                    if retry_value is not None:
                        break
                values.append(retry_value)
            else:
                values.append(value & 0xFFFF)

        return values

if __name__ == '__main__':
    nano = NanoI2C()

    if not nano.is_available():
        print("❌ Arduino not detected on I2C")
        exit(1)

    print("✅ Arduino detected")

    # Single operation examples
    #print("Digital write D2=1:", nano.write(2, 1))
    #print("Digital read D2:", nano.read(2))

    #print("Digital write A2=1:", nano.write(16, 1))
    #print("Digital read A2:", nano.read(16))


    # Batch operation examples
    #print("Read multiple digital:", nano.read_multiple([2]))
    #print("Read multiple digital:", nano.read_multiple([14,15,16,17,20,21]))
    #print("Read multiple digital:", nano.read_multiple([2, 3, 4, 9, 10, 16]))
    #print("Write multiple digital:", nano.write_multiple([(2, 1), (3, 0)]))
    #time.sleep(10)
    for i in range(1000):
        try:
            print("Ultrasonic distances:", nano.ultrasonic_read_multiple([(9, 10), (9, 10), (9, 10), (9, 10), (9, 10)], True))
            #print("Read multiple digital:", nano.read_multiple([14]))
    
        except Exception as e:
            print("Error reading ultrasonic:", e)
            time.sleep(6)

#0-13: Digital pins ✓
#14-17: A0-A3 ✓
#18-19: A4-A5 ✗ (Reserved for I2C SDA/SCL)
#20-21: A6-A7 ✓
