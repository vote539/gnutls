/*
 *      Copyright (C) 2000,2001 Nikos Mavroyanopoulos
 *
 * This file is part of GNUTLS.
 *
 * GNUTLS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GNUTLS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifndef GNUTLS_INT_H

#define GNUTLS_INT_H

#include <defines.h>

/*
#define IO_DEBUG 3 // define this to check non blocking behaviour 
#define BUFFERS_DEBUG
#define HARD_DEBUG
#define WRITE_DEBUG
#define READ_DEBUG
#define HANDSHAKE_DEBUG // Prints some information on handshake 
#define RECORD_DEBUG
#define DEBUG
*/

/* It might be a good idea to replace int with void*
 * here.
 */
#define GNUTLS_SOCKET_PTR int

typedef const int* GNUTLS_LIST;

#define MIN_BITS 1023

#define MAX32 4294967295
#define MAX24 16777215
#define MAX16 65535

/* The sequence of handshake messages should not
 * be larger than this value.
 */
#define MAX_HANDSHAKE_DATA_BUFFER_SIZE 128*1024

#define TLS_RANDOM_SIZE 32
#define TLS_MAX_SESSION_ID_SIZE 32
#define TLS_MASTER_SIZE 48
#define MAX_HASH_SIZE 20

#define MAX_X509_CERT_SIZE 10*1024
#define MAX_LOG_SIZE 1024 /* maximum number of log message */
#define MAX_SRP_USERNAME 256

#define MAX_DNSNAME_SIZE 256

/* The initial size of the receive
 * buffer size. This will grow if larger
 * packets are received.
 */
#define INITIAL_RECV_BUFFER_SIZE 256

/* the default for TCP */
#define DEFAULT_LOWAT 1

/* expire time for resuming sessions */
#define DEFAULT_EXPIRE_TIME 3600

/* the maximum size of encrypted packets */
#define DEFAULT_MAX_RECORD_SIZE 16384
#define RECORD_HEADER_SIZE 5
#define MAX_RECORD_SIZE state->security_parameters.max_record_size
#define MAX_RECV_SIZE 2048+MAX_RECORD_SIZE+RECORD_HEADER_SIZE 	/* 2^14+2048+RECORD_HEADER_SIZE */

#define HANDSHAKE_HEADER_SIZE 4

#include <gnutls_mem.h>
#include <gnutls_ui.h>

#define DECR_LEN(len, x) len-=x; if (len<0) {gnutls_assert(); return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;}
#define DECR_LENGTH_RET(len, x, RET) len-=x; if (len<0) {gnutls_assert(); return RET;}

typedef unsigned char opaque;
typedef struct { opaque pint[3]; } uint24;

#ifdef USE_GCRYPT
# include <gnutls_gcry.h>
#endif

typedef enum crypt_algo { SRPSHA1_CRYPT, BLOWFISH_CRYPT=2 } crypt_algo;
typedef enum ChangeCipherSpecType { GNUTLS_TYPE_CHANGE_CIPHER_SPEC=1 } ChangeCipherSpecType;
typedef enum AlertLevel { GNUTLS_AL_WARNING=1, GNUTLS_AL_FATAL } AlertLevel;
typedef enum AlertDescription { GNUTLS_A_CLOSE_NOTIFY, GNUTLS_A_UNEXPECTED_MESSAGE=10, GNUTLS_A_BAD_RECORD_MAC=20,
			GNUTLS_A_DECRYPTION_FAILED, GNUTLS_A_RECORD_OVERFLOW,  GNUTLS_A_DECOMPRESSION_FAILURE=30,
			GNUTLS_A_HANDSHAKE_FAILURE=40, GNUTLS_A_NETSCAPE_NO_CLIENT_CERTIFICATE=41,
			GNUTLS_A_BAD_CERTIFICATE=42, GNUTLS_A_UNSUPPORTED_CERTIFICATE,
			GNUTLS_A_CERTIFICATE_REVOKED, GNUTLS_A_CERTIFICATE_EXPIRED, GNUTLS_A_CERTIFICATE_UNKNOWN,
			GNUTLS_A_ILLEGAL_PARAMETER, GNUTLS_A_UNKNOWN_CA, GNUTLS_A_ACCESS_DENIED, GNUTLS_A_DECODE_ERROR=50,
			GNUTLS_A_DECRYPT_ERROR, GNUTLS_A_EXPORT_RESTRICTION=60, GNUTLS_A_PROTOCOL_VERSION=70,
			GNUTLS_A_INSUFFICIENT_SECURITY, GNUTLS_A_INTERNAL_ERROR=80, GNUTLS_A_USER_CANCELED=90,
			GNUTLS_A_NO_RENEGOTIATION=100
} AlertDescription;
typedef enum CertificateStatus { GNUTLS_CERT_TRUSTED=1, GNUTLS_CERT_NOT_TRUSTED, GNUTLS_CERT_EXPIRED, GNUTLS_CERT_INVALID, GNUTLS_CERT_NONE } CertificateStatus;
typedef enum CertificateRequest { GNUTLS_CERT_IGNORE, GNUTLS_CERT_REQUEST=1, GNUTLS_CERT_REQUIRE } CertificateRequest;
typedef enum CloseRequest { GNUTLS_SHUT_RDWR=0, GNUTLS_SHUT_WR=1 } CloseRequest;

typedef enum HandshakeState { STATE0=0, STATE1, STATE2, STATE3, STATE4, STATE5,
	STATE6, STATE7, STATE8, STATE9, STATE10, STATE11, STATE20=20, STATE21,
	STATE30=30, STATE31, STATE50=50, STATE60=60, STATE61 } HandshakeState;

typedef enum HandshakeType { GNUTLS_HELLO_REQUEST, GNUTLS_CLIENT_HELLO, GNUTLS_SERVER_HELLO,
		     GNUTLS_CERTIFICATE=11, GNUTLS_SERVER_KEY_EXCHANGE,
		     GNUTLS_CERTIFICATE_REQUEST, GNUTLS_SERVER_HELLO_DONE,
		     GNUTLS_CERTIFICATE_VERIFY, GNUTLS_CLIENT_KEY_EXCHANGE,
		     GNUTLS_FINISHED=20 } HandshakeType;

typedef struct {
	ChangeCipherSpecType type;
} ChangeCipherSpec;

typedef struct {
	opaque * data;
	int size;
} gnutls_datum;
typedef gnutls_datum gnutls_sdatum;

typedef struct {
	AlertLevel level;
	AlertDescription description;
} Alert;


/* STATE */
typedef enum ConnectionEnd { GNUTLS_SERVER=1, GNUTLS_CLIENT } ConnectionEnd;
typedef enum BulkCipherAlgorithm { GNUTLS_CIPHER_NULL=1, GNUTLS_CIPHER_ARCFOUR, GNUTLS_CIPHER_3DES_CBC, GNUTLS_CIPHER_RIJNDAEL_CBC, GNUTLS_CIPHER_TWOFISH_CBC, GNUTLS_CIPHER_RIJNDAEL256_CBC } BulkCipherAlgorithm;
typedef enum Extensions { GNUTLS_EXTENSION_DNSNAME=0, GNUTLS_EXTENSION_MAX_RECORD_SIZE=1, GNUTLS_EXTENSION_SRP=6 } Extensions;
typedef enum KXAlgorithm { GNUTLS_KX_X509PKI_RSA=1, GNUTLS_KX_X509PKI_DHE_DSS, GNUTLS_KX_X509PKI_DHE_RSA, GNUTLS_KX_ANON_DH, GNUTLS_KX_SRP } KXAlgorithm;
typedef enum CredType { GNUTLS_X509PKI=1, GNUTLS_ANON, GNUTLS_SRP } CredType;
typedef enum CipherType { CIPHER_STREAM, CIPHER_BLOCK } CipherType;
typedef enum MACAlgorithm { GNUTLS_MAC_NULL=1, GNUTLS_MAC_MD5, GNUTLS_MAC_SHA } MACAlgorithm;
typedef enum DigestAlgorithm { GNUTLS_DIG_MD5=1, GNUTLS_DIG_SHA } DigestAlgorithm;
typedef enum CompressionMethod { GNUTLS_COMP_NULL=1, GNUTLS_COMP_ZLIB } CompressionMethod;

typedef enum ValidSession { VALID_TRUE, VALID_FALSE } ValidSession;
typedef enum ResumableSession { RESUME_TRUE, RESUME_FALSE } ResumableSession;

/* Record Protocol */
typedef enum ContentType { GNUTLS_CHANGE_CIPHER_SPEC=20, GNUTLS_ALERT, GNUTLS_HANDSHAKE,
		GNUTLS_APPLICATION_DATA } ContentType;

/* STATE (stop) */


/* Pull & Push functions defines: 
 */
typedef ssize_t (*PULL_FUNC)(GNUTLS_SOCKET_PTR, void*, size_t);
typedef ssize_t (*PUSH_FUNC)(GNUTLS_SOCKET_PTR, const void*, size_t);
/* Store & Retrieve functions defines: 
 */
typedef int (*DB_STORE_FUNC)(void*, gnutls_datum key, gnutls_datum data);
typedef int (*DB_REMOVE_FUNC)(void*, gnutls_datum key);
typedef gnutls_datum (*DB_RETR_FUNC)(void*, gnutls_datum key);

typedef struct {
	KXAlgorithm algorithm;
	void* credentials;
	void* next;
} AUTH_CRED;


typedef struct {
	uint8 major;
	uint8 minor;
} ProtocolVersion;

struct GNUTLS_KEY_INT {
	/* For DH KX */
	gnutls_sdatum			key;
	MPI				KEY;
	MPI				client_Y;
	MPI				client_g;
	MPI				client_p;
	MPI				dh_secret;
	/* for SRP */
	MPI				A;
	MPI				B;
	MPI				u;
	MPI				b;
	MPI				a;
	MPI				x;
	/* RSA:                   peer:
	 * modulus is A            a
	 * exponent is B           b
	 * private key is u        x
	 */
	
	/* this is used to hold the peers authentication data 
	 */
	/* AUTH_INFO structures MAY contain malloced 
	 * elements. Check gnutls_session_pack.c, and gnutls_auth.c.
	 * Rememember that this should be calloced!
	 */
	void*				auth_info;
	CredType			auth_info_type;
	int				auth_info_size; /* needed in order to store to db for restoring 
							 */
	uint8				crypt_algo;

	AUTH_CRED*			cred; /* used to specify keys/certificates etc */
	
	int				certificate_requested;
					/* some ciphersuites use this
					 * to provide client authentication.
					 * 1 if client auth was requested
					 * by the peer, 0 otherwise
					 *** In case of a server this
					 * holds 1 if we should wait
					 * for a client certificate verify
					 */
};
typedef struct GNUTLS_KEY_INT* GNUTLS_KEY;


/* STATE (cont) */

#include <gnutls_hash_int.h>
#include <gnutls_cipher_int.h>
#include <gnutls_cert.h>

typedef struct {
	uint8 CipherSuite[2];
} GNUTLS_CipherSuite;

/* Versions should be in order of the oldest
 * (eg. SSL3 is before TLS1)
 */
typedef enum GNUTLS_Version { GNUTLS_SSL3=1, GNUTLS_TLS1, GNUTLS_VERSION_UNKNOWN=0xff } GNUTLS_Version;

/* This structure holds parameters got from TLS extension
 * mechanism. (some extensions may hold parameters in AUTH_INFO
 * structures also - see SRP).
 */

typedef struct {
	opaque 		srp_username[MAX_SRP_USERNAME];
} TLSExtensions;

/* AUTH_INFO structures MUST NOT contain malloced 
 * elements.
 */
 
/* This structure and AUTH_INFO, are stored in the resume database,
 * and are restored, in case of resume.
 * Holds all the required parameters to resume the current 
 * state.
 */

/* if you add anything in Security_Parameters struct, then
 * also modify CPY_COMMON in gnutls_constate.c
 */
typedef struct {
	ConnectionEnd entity;
	KXAlgorithm kx_algorithm;
	/* we've got separate write/read bulk/macs because
	 * there is a time in handshake where the peer has
	 * null cipher and we don't
	 */
	BulkCipherAlgorithm read_bulk_cipher_algorithm;
	MACAlgorithm read_mac_algorithm;
	CompressionMethod read_compression_algorithm;

	BulkCipherAlgorithm write_bulk_cipher_algorithm;
	MACAlgorithm write_mac_algorithm;
	CompressionMethod write_compression_algorithm;

	/* this is the ciphersuite we are going to use 
	 * moved here from gnutls_internals in order to be restored
	 * on resume;
	 */
	GNUTLS_CipherSuite	current_cipher_suite;
	opaque 			master_secret[TLS_MASTER_SIZE];
	opaque 			client_random[TLS_RANDOM_SIZE];
	opaque 			server_random[TLS_RANDOM_SIZE];
	opaque 			session_id[TLS_MAX_SESSION_ID_SIZE];
	uint8 			session_id_size;
	time_t 			timestamp;
	TLSExtensions		extensions;
	uint16			max_record_size;
	GNUTLS_Version		version; /* moved here */
} SecurityParameters;

/* This structure holds the generated keys
 */
typedef struct {
	gnutls_datum server_write_mac_secret;
	gnutls_datum client_write_mac_secret;
	gnutls_datum server_write_IV;
	gnutls_datum client_write_IV;
	gnutls_datum server_write_key;
	gnutls_datum client_write_key;
	int	     generated_keys; /* zero if keys have not
				      * been generated. Non zero
				      * otherwise.
				      */
} CipherSpecs;


typedef struct {
	GNUTLS_CIPHER_HANDLE write_cipher_state;
	GNUTLS_CIPHER_HANDLE read_cipher_state;
	gnutls_datum 	read_mac_secret;
	gnutls_datum 	write_mac_secret;
	uint64		read_sequence_number;
	uint64		write_sequence_number;
} ConnectionState;


typedef struct {
	int* algorithm_priority;
	int algorithms;
} GNUTLS_Priority;

#define BulkCipherAlgorithm_Priority GNUTLS_Priority
#define MACAlgorithm_Priority GNUTLS_Priority
#define KXAlgorithm_Priority GNUTLS_Priority
#define CompressionMethod_Priority GNUTLS_Priority
#define Protocol_Priority GNUTLS_Priority

typedef int x509pki_client_cert_callback_func(struct GNUTLS_STATE_INT*, const gnutls_datum *, int, const gnutls_datum *, int);
typedef int x509pki_server_cert_callback_func(struct GNUTLS_STATE_INT*, const gnutls_datum *, int);

typedef struct {
	opaque				header[HANDSHAKE_HEADER_SIZE];
	/* this holds the number of bytes in the handshake_header[] */
	int				header_size;
	/* this holds the length of the handshake packet */
	int				packet_length;
	HandshakeType			recv_type;
} HANDSHAKE_HEADER_BUFFER;

typedef struct {
	gnutls_datum			buffer;
	gnutls_datum			hash_buffer; /* used to keep all handshake messages */
	gnutls_datum			buffer_handshake; /* this is a buffer that holds the current handshake message */
	ResumableSession		resumable; /* TRUE or FALSE - if we can resume that session */
	HandshakeState			handshake_state; /* holds
					* a number which indicates where
					* the handshake procedure has been
					* interrupted. If it is 0 then
					* no interruption has happened.
					*/
	
	ValidSession			valid_connection; /* true or FALSE - if this session is valid */

	int				may_read; /* if it's 0 then we can read/write, otherwise it's forbiden to read/write
	                                           */
	int				may_write;

	AlertDescription		last_alert; /* last alert received */
	/* this is the compression method we are going to use */
	CompressionMethod		compression_method;
	/* priorities */
	BulkCipherAlgorithm_Priority	BulkCipherAlgorithmPriority;
	MACAlgorithm_Priority		MACAlgorithmPriority;
	KXAlgorithm_Priority		KXAlgorithmPriority;
	CompressionMethod_Priority	CompressionMethodPriority;
	Protocol_Priority		ProtocolPriority;
	
	/* resumed session */
	ResumableSession		resumed; /* TRUE or FALSE - if we are resuming a session */
	SecurityParameters		resumed_security_parameters;

	/* sockets internals */
	int				lowat;

	/* These buffers are used in the handshake
	 * protocol only. freed using _gnutls_clear_handshake_buffers();
	 */
	gnutls_datum 			handshake_send_buffer;
	size_t	 			handshake_send_buffer_prev_size;
	ContentType			handshake_send_buffer_type;
	HandshakeType			handshake_send_buffer_htype;
	ContentType			handshake_recv_buffer_type;
	HandshakeType			handshake_recv_buffer_htype;
	gnutls_datum 			handshake_recv_buffer;
	
					/* this buffer holds a record packet -mostly used for
					 * non blocking IO.
					 */
	gnutls_datum			recv_buffer;
	gnutls_datum			send_buffer; /* holds cached data
					* for the gnutls_write_buffered()
					* function.
					*/ 
	size_t				send_buffer_prev_size; /* holds the
	                                * data written in the previous runs.
	                                */
	size_t				send_buffer_user_size; /* holds the
	                                * size of the user specified data to
	                                * send.
	                                */

					/* 0 if no peeked data was kept, 1 otherwise.
					 */
	int				have_peeked_data;

	/* gdbm */
	char*				db_name;
	int				expire_time;
	struct MOD_AUTH_STRUCT_INT*	auth_struct; /* used in handshake packets and KX algorithms */
	int				v2_hello; /* 0 if the client hello is v3+.
						   * non-zero if we got a v2 hello.
						   */
#ifdef HAVE_LIBGDBM
	GDBM_FILE			db_reader;
#endif
	/* keeps the headers of the handshake packet 
	 */
	HANDSHAKE_HEADER_BUFFER		handshake_header_buffer;
	int				client_certificate_index; /* holds
						* the index of the client
						* certificate to use. -1
						* if none.
						*/
	/* this is the highest version available
	 * to the peer. (advertized version)
	 */
	uint8				adv_version_major;
	uint8				adv_version_minor;

	/* if this is non zero a certificate request message
	 * will be sent to the client. - only if the ciphersuite
	 * supports it.
	 */
	int				send_cert_req;
	int				peer_pk_algorithm;
	/* holds the username got in the srp tls extension
	 */

	/* this is a callback function to call if no appropriate
	 * client certificates were found.
	 */
	x509pki_client_cert_callback_func*	client_cert_callback;
	x509pki_server_cert_callback_func*	server_cert_callback;
	int				x509pki_dhe_bits;
	
	int				max_handshake_data_buffer_size;

	/* PUSH & PULL functions.
	 */
	PULL_FUNC _gnutls_pull_func;
	PUSH_FUNC _gnutls_push_func;
	/* Holds the first argument of PUSH and PULL
	 * functions;
	 */
	int transport_ptr;
	
	/* STORE & RETRIEVE functions. Only used if other
	 * backend than gdbm is used.
	 */
	DB_STORE_FUNC db_store_func;
	DB_RETR_FUNC db_retrieve_func;
	DB_REMOVE_FUNC db_remove_func;
	void* db_ptr;
	
	/* Holds the record size requested by the
	 * user.
	 */
	uint16			proposed_record_size;
	
	/* holds the index of the selected certificate.
	 */
	int			selected_cert_index; 
} GNUTLS_INTERNALS;

struct GNUTLS_STATE_INT {
	SecurityParameters security_parameters;
	CipherSpecs cipher_specs;
	ConnectionState connection_state;
	GNUTLS_INTERNALS gnutls_internals;
	GNUTLS_KEY gnutls_key;
};

typedef struct GNUTLS_STATE_INT *GNUTLS_STATE;




/* functions */
int gnutls_send_alert( GNUTLS_STATE state, AlertLevel level, AlertDescription desc);
int gnutls_PRF( opaque * secret, int secret_size, uint8 * label,
		  int label_size, opaque * seed, int seed_size,
		  int total_bytes, void* ret);
void _gnutls_set_current_version(GNUTLS_STATE state, GNUTLS_Version version);
GNUTLS_Version gnutls_protocol_get_version(GNUTLS_STATE state);
void _gnutls_free_auth_info( GNUTLS_STATE state);

/* These two macros return the advertized TLS version of
 * the peer.
 */
#define _gnutls_get_adv_version_major( state) \
	state->gnutls_internals.adv_version_major

#define _gnutls_get_adv_version_minor( state) \
	state->gnutls_internals.adv_version_minor

#define _gnutls_clear_handshake_buffers( state) \
        gnutls_free( state->gnutls_internals.handshake_send_buffer.data); \
        gnutls_free( state->gnutls_internals.handshake_recv_buffer.data); \
        state->gnutls_internals.handshake_send_buffer.data = NULL; \
        state->gnutls_internals.handshake_recv_buffer.data = NULL; \
        state->gnutls_internals.handshake_send_buffer.size = 0; \
        state->gnutls_internals.handshake_recv_buffer.size = 0; \
        state->gnutls_internals.handshake_send_buffer_prev_size = 0

#endif /* GNUTLS_INT_H */
