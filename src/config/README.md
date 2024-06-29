
# Generating the missing ../config/ Files
.pem, .enc and .key files are removed on the distributed version, one needs to obtain these to have the project working.

### (Ed25519) PEM
Ed25519 is used to generate the digital signature of the interaction with the exchange. 
    This is for Authentication, integrity, Non-repudiation, trust and is a requirement.

The following steps serve to generate the required keys. 

```
apt update
apt install openssl --no.install-recommends
openssl version
```
```
openssl genpkey -algorithm Ed25519 -out ed25519key.pem -aes-256-cbc
openssl pkey -in ed25519key.pem -out ed25519pub.pem -pubout
```

These two pem files are to be placed in ../config/ folder.
The selected password for this files will be the global program password. 

### Obtain exchange Api Key
Follow this steps:
1. Login to your exchange account and select Ed25519 method
2. Upload the ed25519pub.pem contents 
3. Retrive the Exchange Api Key

### Configure exchange Api Key
Follow this steps:
1. Create or clear content in file ../config/aes_salt.enc
2. Create or clear content in file ../config/exchange.key
3. Put plaintext api key (from exchange) into ../config/exchange.key


### ../config/Enviroment.config file
If you decide to change the location of any of these files, make sure to change ../config/enviroment.config file
