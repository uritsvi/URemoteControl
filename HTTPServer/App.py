import http
import threading

import pydantic
from fastapi.responses import JSONResponse

import Constants
import Session


class Client:
    def __init__(self, session: Session.Session):
        self.__session = session

    def get_session(self) -> Session.Session:
        return self.__session


class ClientInfo(pydantic.BaseModel):
    client_type: int


def run_session(session: Session.Session):
    session.run()

    Session.Sessions.get_instance().\
        on_session_terminated(session)


class App:
    __instance = None

    def __init__(self):
        self.__connected = {str: Client}

    @classmethod
    def get_instance(cls):
        if cls.__instance is None:
            cls.__instance = App()

        return cls.__instance

    def handle_connect_new_client(self, hash_value: str, info: ClientInfo) -> int:
        if info.client_type == Constants.CONTROLLER_CLIENT_TYPE:
            return http.HTTPStatus.CONTINUE

        session = \
            Session.Sessions.get_instance().create_new_session()
        if session is None:
            return http.HTTPStatus.INTERNAL_SERVER_ERROR

        self.__connected[hash_value] = Client(session)

        threading.Thread(
            target=run_session,
            args=[session]).\
            start()

        return http.HTTPStatus.CONTINUE

    def handle_disconnect_client(self, hash_value: str, info: ClientInfo):
        if info.client_type == Constants.CONTROLLER_CLIENT_TYPE:
            return

        self.__connected[hash_value] = None

    def get_session_info(self, hash_value: str) -> JSONResponse:

        client = self.__connected.get(hash_value)
        if client is None:
            return JSONResponse(None)

        return JSONResponse(client.get_session().build_dict())

    def try_connect_to_session(self, hash_value: str) -> int:
        client: Client = self.__connected.get(hash_value)
        if client is None:
            return http.HTTPStatus.NOT_FOUND

        session: Session = client.get_session()
        if not session.can_connect():
            return http.HTTPStatus.INTERNAL_SERVER_ERROR

        session.on_new_client_connected()

        return http.HTTPStatus.CONTINUE
