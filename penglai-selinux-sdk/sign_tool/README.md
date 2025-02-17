# penglai sign tool

### 1. 编译：

Penglai sign_tool 需要在安装有OpenSSL 3.0.0的平台上编译（OpenSSL 从 v3.0.0 开始才支持国密算法）。Penglai sign_tool 与 OpenSSL 对接，能够处理用户经OpenSSL 命令行工具生成的标准密钥文件和签名文件。

```
cd sign_tool/ && make
```

获得的二进制为`penglai_sign`



### 2. 使用说明：

使用`penglai_sign` 有下列参数，结合openssl 3.0.0版本可进行单步签名，两步签名，签名材料生成，以及转录签名后的ELF文件中的签名信息，分别对应以下命令。

```
"\nUsage: penglai_sign <commands> [options] file...\n"\
    "Commands:\n"\
    "   sign                    Sign the enclave using the private key\n"\
    "   gendata                 Generate enclave signing material to be signed\n"\
    "   catsig                  Generate the signed enclave with the input signature file, the\n"\
    "                           public key and the enclave signing material\n"\
    "   dump                    Dump metadata information for a signed enclave file\n"\
    "Options:\n"\
    "   -image                  Specify the kernel image file to be signed\n"\
    "   -imageaddr              Specify the kernel image's load address\n"\
    "   -dtb                    Specify the device tree file\n"\
    "   -dtbaddr                Specify the device tree file's load address\n"\
    "                           These four options are required for \"sign\", \"gendata\" and \"catsig\"\n"\
    "   -key                    Specify the key file\n"\
    "                           It is a required option for \"sign\" and \"catsig\"\n"\
    "   -out                    Specify the output file\n"\
    "                           It is a required option for \"sign\", \"gendata\" and \"catsig\"\n"\
    "   -sig                    Specify the signature file for the enclave signing material\n" \
    "                           It is a required option for \"catsig\"\n"\
    "   -unsigned               Specify the enclave signing material generated by \"gendata\"\n" \
    "                           It is a required option for \"catsig\"\n" \
    "   -ccsfile                Specify the Cryper Certificate Struct file to be dumped in human readable form\n"\
    "                           It is a required option for \"dump\"\n"\
    "   -dumpfile               Specify a file to dump Cryper Certificate Struct file in human readable form\n" \
    "                           It is a required option for \"dump\", and a optional option for \"sign\" and \"catsig\"\n" \
    "Run \"penglai_sign -help\" to get this help and exit.\n"
```

#### 2.1 单步签名

Step 1: 单步签名需要应用开发者的SM2私钥，私钥格式支持PEM文件格式。可通过以下命令使用openssl 3.0版本生成。若未安装openssl 或其他命令行工具，可跳过此步，使用`test_dir` 目录下的`SM2PrivateKey.pem` 私钥文件进行尝试。

```
# 生成SM2椭圆曲线参数，保存在 ecp.pem 中
openssl genpkey -genparam -algorithm EC -out ecp.pem \
        -pkeyopt ec_paramgen_curve:sm2 \
        -pkeyopt ec_param_enc:named_curve

# 使用SM2参数生成私钥
openssl genpkey -paramfile ecp.pem -out SM2PrivateKey.pem
```

Step 2: 使用`penglai_sign` 生成开发者数字签名证书`ccs-file`。

```
cd test_dir

../penglai_sign sign \
        -image sec-image -imageaddr 0xc0200000 \
        -dtb sec-dtb.dtb -dtbaddr 0x186000000 \
        -key SM2PrivateKey.pem -out ccs-file
```

#### 2.2 签名信息转录（目前未实现！不支持测试）

Method 1: 在生成签名文件的过程中，可使用`-dumpfile` 参数指定要将签名信息转录到的文件，如单步签名中：

```
../penglai_sign sign \
        -image sec-image -imageaddr 0xc0200000 \
        -dtb sec-dtb.dtb -dtbaddr 0x186000000 \
        -key SM2PrivateKey.pem -out ccs-file -dumpfile dump-file
```

Method 2: 使用dump命令转录已被签名的文件中的签名信息：

```
./penglai_sign dump -ccsfile ccs-file -dumpfile dump-file
```

通过如上方法可得到文件`dump-file` ，其中包含TEE的元数据信息（配置、度量及开发者签名等），采用可阅读的文本形式，可用于向"蓬莱"提交白名单申请。

#### 2.3 两步签名（目前未实现！不支持测试）

两步签名考虑到开发者或应用服务提供商的私钥需要严格保护，不能以明文用于签名工具的输入。

Step 1: 可用openssl 3.0得到SM2密钥对。

```
# 生成SM2椭圆曲线参数，保存在 ecp.pem 中
openssl genpkey -genparam -algorithm EC -out ecp.pem \
        -pkeyopt ec_paramgen_curve:sm2 \
        -pkeyopt ec_param_enc:named_curve

# 使用SM2参数生成私钥
openssl genpkey -paramfile ecp.pem -out SM2PrivateKey.pem

# 使用SM2私钥生成公钥
openssl ec -in SM2PrivateKey.pem -pubout -out SM2PublicKey.pem
```
Step 2: 需要得到待签名enclave 的签名材料，在受保护的环境中如HSM（硬件安全模块）中用私钥进行签名，最后用公钥和签名后的材料来得到签名文件。
```
# 使用签名工具得到待签名enclave 的签名材料
../penglai_sign gendata \
        -image sec-image -imageaddr 0xc0200000 \
        -dtb sec-dtb.dtb -dtbaddr 0x186000000 \
        -out metadata-file

# 通过openssl 3.0用私钥进行签名（验签），此处someid是SM2国标中规定的签名者id，可用邮箱
openssl pkeyutl -sign -in metadata-file -inkey SM2PrivateKey.pem -out sig-file -rawin -digest sm3 \
    -pkeyopt distid:someid
openssl pkeyutl -verify -in metadata-file -inkey SM2PrivateKey.pem -sigfile sig-file \
    -rawin -digest sm3 -pkeyopt distid:someid
```

Step 3: 使用签名文件和公钥生成开发者数字签名证书`ccs-file` ，该命令执行过程使用输入的原镜像文件对签名材料正确性进行验证。

```
./penglai_sign catsig \
        -image sec-image -imageaddr 0xc0200000 \
        -dtb sec-dtb.dtb -dtbaddr 0x186000000 \
        -key SM2PublicKey.pem -unsigned metadata-file -sig sig-file \
        -out ccs-file
```


### 3. 补充openssl 的说明

用openssl 签名/验证 hash：（SM2的hash 要求为32 位byte array）

```
openssl pkeyutl -sign -inkey eckey.pem -in hash-file -out sig-file

openssl pkeyutl -verify -in hash-file -pubin -inkey pub_key.pem -sigfile sig-file
```
