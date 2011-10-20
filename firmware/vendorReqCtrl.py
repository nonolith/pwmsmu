import usb.core

MODE_DISABLED=0
MODE_SVMI=1
MODE_SIMV=2

def unpackSign(n):
	return n - (1<<12) if n>2048 else n

class CEE(object):
	def __init__(self):
		self.init()

	def init(self):
		self.dev = usb.core.find(idVendor=0x9999, idProduct=0xffff)
		if not self.dev:
			raise IOError("device not found")
			
		self.dev.set_configuration()

	def getValue(self):
		data = self.dev.ctrl_transfer(0x40|0x80, 0xA0, 0, 0, 2)
		data = data[1] << 8 | data[0]
		print hex(data)

	def blinkLED(self):
		self.dev.ctrl_transfer(0x40|0x80, 0x73, 0, 0, 0)	

if __name__ == "__main__":
	cee = CEE()
	while True:
		cee.blinkLED()
		cee.getValue()
