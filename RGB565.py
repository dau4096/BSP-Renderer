"RGB565.py"


def binary(value:int) -> None: return "0b" + str(bin(value))[2:].zfill(16);
def hexadecimal(value:int) -> None: return "0x" + str(hex(value))[2:].zfill(4);


def convertRGBto565(r:int, g:int, b:int) -> int:
	#Convert individual components to RGB565 value
	return (
		((r & 0xF8) << 8) |
		((g & 0xFC) << 3) |
		((b & 0xF8) >> 3)
	);


def convert565toRGB(rgb565:int) -> tuple[int, int, int]:
	return (
		(rgb565 >> 8) & 0xF8, #R
		(rgb565 >> 3) & 0xFC, #G
		(rgb565 << 3) & 0xF8  #B
	);



print(binary(convertRGBto565(  0,   0,   0))); #RGB_BLACK
print(binary(convertRGBto565(127, 127, 127))); #RGB_GREY
print(binary(convertRGBto565(255, 255, 255))); #RGB_WHITE
print(binary(convertRGBto565(255,   0,   0))); #RGB_RED
print(binary(convertRGBto565(  0, 255,   0))); #RGB_GREEN
print(binary(convertRGBto565(  0,   0, 255))); #RGB_BLUE
print(binary(convertRGBto565(255, 255,   0))); #RGB_YELLOW
print(binary(convertRGBto565(  0, 255, 255))); #RGB_CYAN
print(binary(convertRGBto565(255,   0, 255))); #RGB_MAGENTA
print();
print(hexadecimal(convertRGBto565(  0,   0,   0))); #RGB_BLACK
print(hexadecimal(convertRGBto565(127, 127, 127))); #RGB_GREY
print(hexadecimal(convertRGBto565(255, 255, 255))); #RGB_WHITE
print(hexadecimal(convertRGBto565(255,   0,   0))); #RGB_RED
print(hexadecimal(convertRGBto565(  0, 255,   0))); #RGB_GREEN
print(hexadecimal(convertRGBto565(  0,   0, 255))); #RGB_BLUE
print(hexadecimal(convertRGBto565(255, 255,   0))); #RGB_YELLOW
print(hexadecimal(convertRGBto565(  0, 255, 255))); #RGB_CYAN
print(hexadecimal(convertRGBto565(255,   0, 255))); #RGB_MAGENTA