#define PRIVATE_KEY_SIZE       32
#define PUBLIC_KEY_SIZE        64
#define HASH_SIZE              32
#define SIGNATURE_SIZE         64
#define TEE_CUSTOM_FIELD_SIZE  512

struct custom_message_t
{
    unsigned char magic[16];
    unsigned char ipaddr_info[128];
    unsigned char pubkey_footprint[128];
};

struct tee_sig_message_t
{
    unsigned char hash[HASH_SIZE];
    unsigned char custom_field[TEE_CUSTOM_FIELD_SIZE];
    unsigned long nonce;
};

struct tee_report_t
{
    struct tee_sig_message_t sig_message;
    unsigned char signature[SIGNATURE_SIZE];
    unsigned char sm_pub_key[PUBLIC_KEY_SIZE];
};

struct prikey_t
{
  unsigned char dA[PRIVATE_KEY_SIZE];
};

struct pubkey_t
{
  unsigned char xA[PUBLIC_KEY_SIZE/2];
  unsigned char yA[PUBLIC_KEY_SIZE/2];
};

struct signature_t
{
    unsigned char r[PUBLIC_KEY_SIZE/2];
    unsigned char s[PUBLIC_KEY_SIZE/2];
};
