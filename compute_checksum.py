def compute(string):
    checksum = 0
    for c in string:
        checksum = checksum ^ ord(c)
    return checksum

checksum = compute("Hello World! Rock on buddy!")
checksum = compute("zzzzzzzzzzzzzzzzzzz")
print(checksum)
