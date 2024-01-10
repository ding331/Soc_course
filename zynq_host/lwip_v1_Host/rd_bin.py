#isu 1 : debug as-> run as -> memory
#isu 2 : define global cont
# PPT
# arch
# 0x01451801
file_path = 'C:/Users/C110/PycharmProjects/Soc/road720.bin'
Address = 0;
try:
    # Open the binary file in read mode
    with open(file_path, 'rb') as file:
        # Read the first 16 bytes and print their byte value, hexadecimal representation, and address
        bytes_read = file.read( )
        for i, byte in enumerate(bytes_read):
            if Address != 16:   #345600+8:
                print(f"Address: {Address}, Byte: {byte:02X}, Hex: {hex(byte)}")
                Address = Address + 1;



except FileNotFoundError:
    print("File not found.")
except Exception as e:
    print("An error occurred:", e)
