import subprocess
import os

# print(os.getcwd())
# os.chdir("build/")
commandList = []
commandList.append("./Echoer --redisIP 192.168.1.102")
commandList.append("./Echoer --redisIP 192.168.1.102")
commandList.append("./Echoer --redisIP 192.168.1.102")
commandList.append("./Echoer --redisIP 192.168.1.102")
commandList.append("./Echoer --redisIP 192.168.1.102")


for command in commandList:
    subprocess.Popen(command.split(), cwd="build/examples")


input("Press Enter to shutdown...\n")
os.system("pkill -f Echoer")
os.system("pkill -f Initiator")
