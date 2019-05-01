# metahash-eth-client

### Building:
```
apt install rapidjson-dev libgmp-dev libcurl4-gnutls-dev

cd /tmp/
git clone https://github.com/adsniper/metahash-eth-client.git

cd metahash-eth-client/3rd_party/secp256k1
./autogen.sh
./configure --enable-module-recovery --with-bignum=no
make
cp ./.libs/libsecp256k1.a ./lib/linux/
cd ../gmp
make clean
make
cp .libs/libgmp.a ./lib/
cp .libs/libgmpxx.a ./lib/
cd ../cryptopp
make
make install
cd ..

mkdir build
cd build
cmake ..
make 
```

### Using:
```
run --help what would see allowed options
run --request what would will see description for requests
```

### Requests:
```
Generate wallet 
{"id":decimal, "version":"2.0","method":"generate", "params":{"password":"str"}}

Balance of wallet 
{"id":decimal, "version":"2.0","method":"fetch-balance", "params":{"address":"hexstr"}}

History of wallet 
{"id":decimal, "version":"2.0","method":"fetch-history", "params":{"address":"hexstr"}}

Create transaction 
{"id":decimal, "version":"2.0","method":"create-tx", "params":{"address":"hexstr", "password":"str", "to":"hexstr", "value":"decimal/all", "fee":"decimal/auto", "nonce":"decimal", "isPending": "bool"}}

Send transaction 
{"id":decimal, "version":"2.0","method":"send-tx", "params":{"address":"hexstr", "password":"str", "to":"hexstr", "value":"decimal/all", "fee":"decimal/auto", "nonce":"decimal", "isPending": "bool"}}
# nonce can be not specified, then in will be calculated automatically

Create transaction token
{"id":decimal, "version":"2.0","method":"create-tx-token", "params":{"address":"hexstr", "password":"str", "to":"hexstr", "token": "tokenAddr", "value":"decimal/all", "fee":"decimal/auto", "nonce":"decimal", "isPending": "bool"}}

Send transaction token
{"id":decimal, "version":"2.0","method":"send-tx-token", "params":{"address":"hexstr", "password":"str", "to":"hexstr", "token": "tokenAddr", "value":"decimal/all", "fee":"decimal/auto", "nonce":"decimal", "isPending": "bool"}}
# nonce can be not specified, then in will be calculated automatically

Management of batch transactions

{"id":decimal, "version":"2.0","method":"add-addresses-to-batch", "params":{"group": "group", "address":"hexstr"}}
{"id":decimal, "version":"2.0","method":"del-addresses-to-batch", "params":{"group": "group", "address":"hexstr"}}
{"id":decimal, "version":"2.0","method":"get-addresses-to-batch", "params":{"group": "group"}}
{"id":decimal, "version":"2.0","method":"add-addresses-to-batch-tkn", "params":{"group": "group", "address":"hexstr"}}
{"id":decimal, "version":"2.0","method":"del-addresses-to-batch-tkn", "params":{"group": "group", "address":"hexstr"}}
{"id":decimal, "version":"2.0","method":"get-addresses-to-batch-tkn", "params":{"group": "group"}}

{"id":decimal, "version":"2.0","method":"batch-balance", "params":{"group": "group", "block": blockInt}}
{"id":decimal, "version":"2.0","method":"batch-history", "params":{"group": "group", "block": blockInt}}
{"id":decimal, "version":"2.0","method":"batch-balance-tkn", "params":{"group": "group", "block": blockInt}}
{"id":decimal, "version":"2.0","method":"batch-history-tkn", "params":{"group": "group", "block": blockInt}}

ADRESSES TRACKING

Add address to tracking list
{"id":decimal, "version":"2.0","method":"add-to-tracking", "params":{"address":"hexstr"}}

Remove address from tracking list
{"id":decimal, "version":"2.0","method":"del-from-tracking", "params":{"address":"hexstr"}}

Get tracking list
{"id":decimal, "version":"2.0","method":"get-tracking"}

```

