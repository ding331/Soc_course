#isu 1 : debug as-> run as -> memory
#isu 2 : define global cont
# PPT
# arch
# 0x01451801
# file_path = 'C:/Users/C110/PycharmProjects/Soc/road720.bin'
file_path = 'C:/Users/C110/PycharmProjects/Soc/rec_opt_for_test/rec_for_test.bin'
Address = 0
whitePixel_cnt = 0
page = 1
try:
    # Open the binary file in read mode
    with open(file_path, 'rb') as file:
        # Read the first 16 bytes and print their byte value, hexadecimal representation, and address
        bytes_read = file.read( )
        for i, byte in enumerate(bytes_read):
            if i % 345600 == 0 and i != 0:
                Address = 0
                print("white pixel area: ", whitePixel_cnt)
                whitePixel_cnt = 0
                page += 1
            else:
                Address = Address + 1
            # if byte != 0 and 345600 <= i <= 134476 + 345600:
            if byte != 0 and int(Address / 720) <= 185 and page == 2:   #345600+8:
                whitePixel_cnt += 1
                print(f"page: {page}, Address: {int(Address % 720), int(Address / 720)}, Byte: {byte:02X}, Hex: {hex(byte)}")

except FileNotFoundError:
    print("File not found.")
except Exception as e:
    print("An error occurred:", e)
