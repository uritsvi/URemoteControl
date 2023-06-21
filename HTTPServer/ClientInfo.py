import pydantic

import UserToken

CLIENT_TYPE_CONTROLLED = 1
CLIENT_TYPE_CONTROLLER = 2


class ClientInfo(pydantic.BaseModel):
    client_type: str

    public_token: str
    private_token: str

    def build_tokens_object(self) -> UserToken.UserToken:
        out = UserToken.UserToken()

        out.public_token = self.public_token
        out.private_token = self.private_token

        return out
