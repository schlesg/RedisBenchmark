import subprocess
import os

# print(os.getcwd())
# os.chdir("build/")
commandList = []
commandList.append("./Initiator --SubTopic dummy --PubTopic MS1 --msgLength 5000 --pubName Dummy --updateRate 100") #C1
commandList.append("./Initiator --SubTopic dummy --PubTopic MS1 --msgLength 5000 --pubName Dummy --updateRate 100") #C2
commandList.append("./Initiator --SubTopic dummy --PubTopic MS1 --msgLength 5000 --pubName Dummy --updateRate 100") #C3
commandList.append("./Initiator --SubTopic dummy --PubTopic MS1 --msgLength 5000 --pubName Dummy --updateRate 100") #C4
commandList.append("./Echoer --SubTopic MS1 --PubTopic MS2 ") #MS1
commandList.append("./Echoer --SubTopic MS2 --PubTopic C5 ") #MS2
commandList.append("./Echoer --SubTopic C5 --PubTopic dummy ") #GW1
commandList.append("./Echoer --SubTopic C5 --PubTopic dummy ") #GW2
commandList.append("./Echoer --SubTopic C5 --PubTopic dummy ") #GW3

# commandList.append("./Initiator --SubTopic C5  --PubTopic MS1 --msgLength 5000") #C5

for command in commandList:
    subprocess.Popen(command.split(), cwd="build/examples")


input("Press Enter to shutdown...\n")
os.system("pkill -f Echoer")
os.system("pkill -f Initiator")
