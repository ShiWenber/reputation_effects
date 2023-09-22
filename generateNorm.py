# python 生成 0-15 的二进制的四位turple
# 生成0到15的二进制的四位元组
result = [(bin(i)[2:]).zfill(4) for i in range(16)]
print(result)

for i in range(result.__len__()):
    result[i] = list(result[i])
    fileName = 'norm' + str(i) + '.csv'
    file = open(fileName, 'w')
    file.write("C,C,D,D\n")
    file.write("C,D,C,D\n")
    # 将 result[i] 用 , 连接起来
    temp_str = ','.join(result[i])
    file.write(temp_str)
    file.close()