import threading
import typing
import os
import subprocess

import Constants


class Session:
    def __init__(self, port):
        self.__port = port
        self.__is_running = False
        self.__num_of_connected = 0

    def get_port(self):
        return self.__port

    def run(self):
        path = os.path.join(Constants.GO_SERVER_FOLDER_NAME, Constants.GO_SERVER_FILE_NAME)

        command = [path, str(self.__port)]

        subprocess.Popen(
            command,
            creationflags=subprocess.CREATE_NEW_CONSOLE).wait()

    def build_dict(self) -> {str, int}:
        out = {Constants.SESSION_PORT_DICT_ENTRY: self.__port}

        return out

    def get_is_running(self) -> bool:
        return self.__is_running

    def on_new_client_connected(self):
        self.__num_of_connected += 1

    def can_connect(self) -> bool:
        if self.__num_of_connected is Constants.NUM_OF_CLIENTS_IN_SESSION:
            return False
        return True


class Sessions:
    __instance = None

    def __init__(self):
        self.__free_ports = list(range(0, 100))
        self.__mutex = threading.Lock()

    @classmethod
    def get_instance(cls):
        if cls.__instance is None:
            cls.__instance = Sessions()

        return cls.__instance

    def create_new_session(self) -> typing.Optional[Session]:
        self.__mutex.acquire()

        if len(self.__free_ports) == 0:
            return None

        port = self.__free_ports.pop() + Constants.FIRST_PORT_OFFSET

        print(port)

        self.__mutex.release()

        return Session(port)

    def on_session_terminated(self, session: Session):
        self.__mutex.acquire()

        self.__free_ports.append(session.get_port() - Constants.FIRST_PORT_OFFSET)

        self.__mutex.release()
