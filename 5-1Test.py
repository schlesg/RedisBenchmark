import subprocess
import os

# print(os.getcwd())
# os.chdir("build/")
commandList = []
commandList.append("./Echoer")
commandList.append("./Initiator --msgLength 1000")
commandList.append("./Initiator --pubName Dummy --msgLength 1000")
commandList.append("./Initiator --pubName Dummy --msgLength 1000")
commandList.append("./Initiator --pubName Dummy --msgLength 1000")
commandList.append("./Initiator --pubName Dummy --msgLength 1000")


# commandList.append("./initiator --SubTopic C5 --PubTopic MS1 --msgLength 5000 --roundtripCount 1000") #C5

for command in commandList:
    subprocess.Popen(command.split(), cwd="build/examples")


input("Press Enter to shutdown...\n")
os.system("pkill -f Echoer")
os.system("pkill -f Initiator")
