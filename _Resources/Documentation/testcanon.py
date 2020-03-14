import serial
import struct
import sys

ser = serial.Serial(
    port='/dev/ttyUSB1',
    baudrate=19200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=0.1
)

print(ser.isOpen())

##02H 00H 00H 00H 00H 02H
#thestring = "\x02\x00\x00\x00\x00\x02"
##\x7E\xFF\x03\x00\x01\x00\x02\x0A\x01\xC8\x04\xD0\x01\x02\x80\x00\x00\x00\x00\x8E\xE7\z7E"
#data = struct.pack(hex(thestring))
#data = struct.pack(hex, 0x7E, 0xFF, 0x03, 0x00, 0x01, 0x00, 0x02, 0x0A, 0x01, 0xC8,      0x04, 0xD0, 0x01, 0x02, 0x80, 0x00, 0x00, 0x00, 0x00, 0x8E, 0xE7, 0x7E)
#ser.write(data)
#s = ser.read(1)
#print(s)
#ser.close()

# Pinout:
# MiniDIN8 https://static.quade.co/wp-content/uploads/2018/05/OfS0683.p_10-foot-MiniDIN-8-Pin-Male-to-Female-Extension-Cable-for-Gaming-Analog-Joysticks.gif
# RS232      MiniDIN8
# 2 RX -->   1 TX
# 5 GND -->  4 GND
# 3 TX	-->  7 RX

arguments = len(sys.argv) - 1
if arguments != 1:
	print "Missing arguments!"
	sys.exit()

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
ser.write(packet)
while True:
  s = ser.read(1)
  if len(s) != 1: 
    break
  print(hex(ord(s))),
ser.close();


