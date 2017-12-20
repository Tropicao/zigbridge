import os
import requests
import uuid
import json

class Bridge:

    def __init__(self):
        self.address = "192.168.0.15"
        self.load_username_from_file()

    def load_username_from_file(self):
        path = os.getcwd() + "/username.txt"
        if (os.path.isfile(path) ):
            with open(path, 'r') as f:
                self.username = f.read()
                f.close()
            print('Username loaded from file : ', self.username)
        else:
            print("No username file found")
            self.get_new_username()

    def save_username_to_file(self):
        path = os.getcwd() + "/username.txt"
        with open(path, 'w') as f:
            f.write(self.username)
            f.close()


    def get_new_username(self):
        path = "http://"+self.address+"/api"
        app_name = uuid.uuid4().hex
        print('Push bridge button, then ENTER')
        input()
        r = requests.post(path, data=json.dumps({"devicetype":app_name}))
        print(r.json)
        resp = r.json()
        self.username = resp[0]['success']['username']
        print('New username : ', self.username)
        self.save_username_to_file()

    def get_config(self):
        path = "http://" + self.address + "/api/" + self.username
        r = requests.get(path)
        print(r.json())

    def get_lights(self):
        path = "http://" + self.address + "/api/" + self.username + "/lights"
        r = requests.get(path)
        print(r.json())

    def set_touchlink(self):
        path = "http://"+self.address+"/api/"+self.username+"/config"
        print('Push bridge button, then ENTER')
        input()
        r = requests.put(path, data=json.dumps({"touchlink":True}))
        print(r.json)

    def delete_light(self, value):
        path = "http://"+self.address+"/api/"+self.username+"/lights/"+str(value)
        r = requests.delete(path)
        print(r.json)

if __name__ == '__main__':
    bridge = Bridge()
    bridge.get_lights()

