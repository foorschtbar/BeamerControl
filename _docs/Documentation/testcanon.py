import serial
import struct
import sys

# Setup serial connection
ser = serial.Serial(
	port='/dev/ttyUSB0',				# USB-to-RS232-Converter
	baudrate=19200,						# Baud rate 19200 bps 
	parity=serial.PARITY_NONE,			# Parity bit:   No parity 
	stopbits=serial.STOPBITS_ONE,		# Stop bits:    1 bit 
	bytesize=serial.EIGHTBITS,			# Data length:  8 bits 
	timeout=0.1
)

print("print("Serial isOpen:", ser.isOpen())", ser.isOpen())

# Parse arguments
arguments = len(sys.argv) - 1
if arguments != 1:
	print "Missing arguments!"
	sys.exit()

# Create command
packet = bytearray()
arg = sys.argv[1].lower().strip()
if arg == "on":
	print "send on"
	packet.append(0x02)
	packet.append(0x00)
	packet.append(0x00)
	packet.append(0x00)
	packet.append(0x00)
	packet.append(0x02)
elif arg == "off":
	print "send off"
	packet.append(0x02)
	packet.append(0x01)
	packet.append(0x00)
	packet.append(0x00)
	packet.append(0x00)
	packet.append(0x03)
elif arg == "status":
	print "send status"
	packet.append(0x00)
	packet.append(0xbf)
	packet.append(0x00)
	packet.append(0x00)
	packet.append(0x01)
	packet.append(0x02)
	packet.append(0xc2)
elif arg == "showoff":
	print "send showoff"
	packet.append(0x02)
	packet.append(0x10)
	packet.append(0x00)
	packet.append(0x00)
	packet.append(0x00)
	packet.append(0x12)
elif arg == "showon":
	print "send showon"
	packet.append(0x02)
	packet.append(0x11)
	packet.append(0x00)
	packet.append(0x00)
	packet.append(0x00)
	packet.append(0x13)
else:
	print "unkown command:",arg
	sys.exit();
	
# Send command
ser.write(packet)

# Read response
while True:
	s = ser.read(1)
	if len(s) != 1: 
		break
	print("{:02X}".format(ord(s))),

# Close serial connection
ser.close();