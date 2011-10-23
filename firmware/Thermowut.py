import usb.core

class Thermowut(object):
	unTwos = lambda self, n: n - (1<<12) if n>2048 else n   

	def __init__(self):
		self.init()

	def init(self):
		self.dev = usb.core.find(idVendor=0x9999, idProduct=0xffff)
		if not self.dev:
			raise IOError("device not found")
			
		self.dev.set_configuration()

	def getValue(self):
		data = self.dev.ctrl_transfer(0x40|0x80, 0xA0, 0, 0, 4)
		return self.unTwos((data[2] << 8 | data[3]) >> 4)

	def toggleLED(self):
		self.dev.ctrl_transfer(0x40|0x80, 0x73, 0, 0, 0)	

if __name__ == "__main__":
	thermowut = Thermowut()
	while True:
		thermowut.getValue()
