import http.client

import uvicorn
import fastapi
import Constants

import Utils
import App

app = fastapi.FastAPI()


@app.post(Constants.CONNECT_ROUT)
async def connect(request: fastapi.Request, client_info: App.ClientInfo):
    hash_value = \
        request.headers.get(Constants.AUTHORIZATIONE_HEADER_NAME)

    Utils.log("connect " +
              hash_value +
              " " +
              str(client_info))

    App.App.get_instance().handle_connect_new_client(
        hash_value,
        client_info)

    return http.HTTPStatus.CONTINUE


@app.post(Constants.DISCONNECT_ROUT)
async def disconnect(request: fastapi.Request, client_info: App.ClientInfo):
    hash_value = \
        request.headers.get(Constants.AUTHORIZATIONE_HEADER_NAME)

    Utils.log(
        "disconnect " +
        hash_value +
        " " +
        str(client_info))

    App.App.get_instance().handle_disconnect_client(
        hash_value,
        client_info)

    return http.HTTPStatus.CONTINUE


@app.get(Constants.GET_SESSION_INFO)
async def get_session_info(hash_value: str):

    return App.App.get_instance().get_session_info(hash_value)


@app.post(Constants.ON_CONNECT_TO_SESSION)
async def try_connect_to_session(hash_value: str):

    return App.App.get_instance().try_connect_to_session(hash_value)


def serve():
    uvicorn.run(app, host="0.0.0.0", port=8080)


if __name__ == "__main__":
    serve()
