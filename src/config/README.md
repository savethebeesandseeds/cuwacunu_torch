
# Generating the missing ./config/ Files
.pem, .enc and .key files are removed on the distributed version, one needs to obtain these to have the project working.

### (Ed25519) PEM
Ed25519 is used to generate the digital signature of the interaction with the exchange. 
    This is for Authentication, integrity, Non-repudiation, trust and is a requirement.

The following steps serve to generate the required keys. 

```
apt update
apt install openssl --no-install-recommends
openssl version
```
```
cd ./src/config
openssl genpkey -algorithm Ed25519 -out ed25519key.pem -aes-256-cbc
openssl pkey -in ed25519key.pem -out ed25519pub.pem -pubout
```

These two pem files are to be placed in ./src/config/ folder.
The selected password for this files will be the global program password. 

Check the ./src/config/.config file for theses paths correctly configured

### Obtain exchange Api Key
You can retrive the API_KEY from the main page at Binance, 
or log in into the testnet: https://testnet.binance.vision/

Follow this steps:
1. Login to your exchange account and select Ed25519 method
2. Upload the ed25519pub.pem contents 
3. Retrive the Exchange Api Key

### Configure exchange Api Key
Follow this steps:
1. Create a new empty file named ./config/aes_salt.enc
2. Put plaintext api_key (retrived from exchange) into file ./config/exchange.key

### Websocket API_KEY
There is another api_key that is retrived when calling binance's "session.logon" method, 
for the Websocket Protocol. This is separated, but the above mentioned api_key is still 
needed to retrive the websocket api_key. 
