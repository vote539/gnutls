/*
 *      Copyright (C) 2000 Nikos Mavroyanopoulos
 *
 * This file is part of GNUTLS.
 *
 *  The GNUTLS library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public   
 *  License as published by the Free Software Foundation; either 
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */

#include <gnutls_int.h>
#include "gnutls_errors.h"
#include <libtasn1.h>
#ifdef STDC_HEADERS
# include <stdarg.h>
#endif

extern void (*_gnutls_log_func)( const char*);


struct gnutls_error_entry {
	const char *name;
	int  number;
	int  fatal;
};
typedef struct gnutls_error_entry gnutls_error_entry;

static gnutls_error_entry error_algorithms[] = {
	{ "Success.", GNUTLS_E_SUCCESS, 0 }, 
	{ "Could not negotiate a supported cipher suite.", GNUTLS_E_UNKNOWN_CIPHER_SUITE, 1 }, 
	{ "The certificate and the given key do not match.", GNUTLS_E_CERTIFICATE_KEY_MISMATCH, 1 }, 
	{ "Could not negotiate a supported compression method.", GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM, 1 }, 
	{ "An unknown public key algorithm was encountered.", GNUTLS_E_UNKNOWN_PK_ALGORITHM, 1 }, 
	{ "Unspecified error.", GNUTLS_E_UNKNOWN_ERROR, 1 }, 

	{ "An algorithm that is not enabled was negotiated.", GNUTLS_E_UNWANTED_ALGORITHM, 1 }, 
	{ "A large TLS record packet was received.", GNUTLS_E_LARGE_PACKET, 1 }, 
	{ "A record packet with illegal version was received.", GNUTLS_E_UNSUPPORTED_VERSION_PACKET, 1 }, 
	{ "The Diffie Hellman prime sent by the server is not acceptable (not long enough).", GNUTLS_E_DH_PRIME_UNACCEPTABLE, 1 }, 
	{ "A TLS packet with unexpected length was received.", GNUTLS_E_UNEXPECTED_PACKET_LENGTH, 1 }, 
	{ "The specified session has been invalidated for some reason.", GNUTLS_E_INVALID_SESSION, 1 }, 

	{ "GnuTLS internal error.", GNUTLS_E_INTERNAL_ERROR, 1 }, 
	{ "An Illegal TLS extension was received.", GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION, 1 }, 
	{ "A TLS Fatal alert has been received.", GNUTLS_E_FATAL_ALERT_RECEIVED ,1 }, 
	{ "An unexpected TLS packet was received.", GNUTLS_E_UNEXPECTED_PACKET, 1 }, 
	{ "A TLS Warning alert has been received.", GNUTLS_E_WARNING_ALERT_RECEIVED, 0 }, 
	{ "An error was encountered at the TLS Finished packet calculation.", GNUTLS_E_ERROR_IN_FINISHED_PACKET, 1 }, 
	{ "The peer did not send any certificate.", GNUTLS_E_NO_CERTIFICATE_FOUND, 1 }, 

	{ "No temporary RSA parameters were found. You should generate them.", GNUTLS_E_NO_TEMPORARY_RSA_PARAMS, 1 }, 
	{ "An unexpected TLS handshake packet was received.", GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET, 1 }, 
	{ "The scanning of a large integer has failed.", GNUTLS_E_MPI_SCAN_FAILED, 1 }, 
	{ "The printing of a large integer has failed.", GNUTLS_E_MPI_PRINT_FAILED, 1 }, 
	{ "Decryption of the TLS record packet has failed.", GNUTLS_E_DECRYPTION_FAILED, 1 }, 
	{ "Encryption of the TLS record packet has failed.", GNUTLS_E_ENCRYPTION_FAILED, 1 }, 
	{ "Public key decryption has failed.", GNUTLS_E_PK_DECRYPTION_FAILED, 1 }, 
	{ "Public key encryption has failed.", GNUTLS_E_PK_ENCRYPTION_FAILED, 1 }, 
	{ "Public key signing has failed.", GNUTLS_E_PK_SIGNATURE_FAILED, 1 }, 
	{ "Decompression of the TLS record packet has failed.", GNUTLS_E_DECOMPRESSION_FAILED, 1 }, 
	{ "Compression of the TLS record packet has failed.", GNUTLS_E_COMPRESSION_FAILED, 1 }, 

	{ "Internal error in memory allocation.", GNUTLS_E_MEMORY_ERROR, 1 }, 
	{ "An unimplemented feature has been requested.", GNUTLS_E_UNIMPLEMENTED_FEATURE, 1 }, 
	{ "Insuficient credentials for that request.", GNUTLS_E_INSUFICIENT_CREDENTIALS, 1 }, 
	{ "Error in password file.", GNUTLS_E_PWD_ERROR, 1 }, 
	{ "Wrong padding in PKCS1 packet.", GNUTLS_E_PKCS1_WRONG_PAD, 1 }, 
	{ "The requested session has expired.", GNUTLS_E_EXPIRED, 1 }, 
	{ "Hashing has failed.", GNUTLS_E_HASH_FAILED, 1 }, 
	{ "Parsing error.", GNUTLS_E_PARSING_ERROR, 1 }, 
	{ "The requested data, were not available.", GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE, 0 }, 
	{ "Error in the pull function.", GNUTLS_E_PULL_ERROR, 1 }, 
	{ "Error in the push function.", GNUTLS_E_PUSH_ERROR, 1 }, 
	{ "The upper limit in record packet sequence number has been reached. Wow!", GNUTLS_E_RECORD_LIMIT_REACHED, 1 }, 
	{ "Error in the X.509 certificate.", GNUTLS_E_X509_CERTIFICATE_ERROR, 1 }, 
	{ "Unknown Subject Alternative name in X.509 certificate.", GNUTLS_E_X509_UNKNOWN_SAN, 1 }, 

	{ "Unsupported critical extension in X.509 certificate.", GNUTLS_E_X509_UNSUPPORTED_CRITICAL_EXTENSION, 1 }, 
	{ "Key usage violation in X.509 certificate, has been detected.", GNUTLS_E_X509_KEY_USAGE_VIOLATION, 1 }, 
	{ "Function was interrupted.", GNUTLS_E_AGAIN, 0 }, 
	{ "Function was interrupted.", GNUTLS_E_INTERRUPTED, 0 }, 
	{ "Rehandshake was requested by the peer.", GNUTLS_E_REHANDSHAKE, 0 },
	{ "TLS Application data were received, while expected handshake data.", GNUTLS_E_GOT_APPLICATION_DATA, 1 }, 
	{ "Error in Database backend.", GNUTLS_E_DB_ERROR, 1 }, 
	{ "The certificate type is not supported.", GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE, 1 }, 
	{ "Invalid parameters given in request.", GNUTLS_E_INVALID_PARAMETERS, 1 }, 
	{ "The request is invalid.", GNUTLS_E_INVALID_REQUEST, 1 }, 
	{ "An illegal parameter has been received.", GNUTLS_E_ILLEGAL_PARAMETER, 1 }, 
	{ "Error in file.", GNUTLS_E_FILE_ERROR, 1 }, 
	{ "Error in ASCII armoring.", GNUTLS_E_ASCII_ARMOR_ERROR, 1 }, 

	{ "ASN1: Element was not found.", GNUTLS_E_ASN1_ELEMENT_NOT_FOUND, 1 }, 
	{ "ASN1: Identifier was not found", GNUTLS_E_ASN1_IDENTIFIER_NOT_FOUND, 1 }, 
	{ "ASN1: Error in DER parsing.", GNUTLS_E_ASN1_DER_ERROR, 1 }, 
	{ "ASN1: Value was not found.", GNUTLS_E_ASN1_VALUE_NOT_FOUND, 1 }, 
	{ "ASN1: Generic parsing error.", GNUTLS_E_ASN1_GENERIC_ERROR, 1 }, 
	{ "ASN1: Value is not valid.", GNUTLS_E_ASN1_VALUE_NOT_VALID, 1 }, 
	{ "ASN1: Error in TAG.", GNUTLS_E_ASN1_TAG_ERROR, 1 }, 
	{ "ASN1: error in implicit tag", GNUTLS_E_ASN1_TAG_IMPLICIT, 1 }, 
	{ "ASN1: Error in type 'ANY'.", GNUTLS_E_ASN1_TYPE_ANY_ERROR, 1 }, 
	{ "ASN1: Syntax error.", GNUTLS_E_ASN1_SYNTAX_ERROR, 1 }, 
	{ "ASN1: Overflow in DER parsing.", GNUTLS_E_ASN1_DER_OVERFLOW, 1 }, 

	{ "Too many empty record packets have been received.", GNUTLS_E_TOO_MANY_EMPTY_PACKETS, 1 }, 
	{ "The initialization of GnuTLS-extra has failed.", GNUTLS_E_INIT_LIBEXTRA, 1 }, 
	{ "The GnuTLS library version does not match the GnuTLS-extra library version.", 
		GNUTLS_E_LIBRARY_VERSION_MISMATCH, 1 }, 
	{ "The specified GnuPG TrustDB version is not supported. TrustDB v4 is supported.", 
		GNUTLS_E_OPENPGP_TRUSTDB_VERSION_UNSUPPORTED, 1 },
	{ "The initialization of LZO has failed.", GNUTLS_E_LZO_INIT_FAILED, 1 }, 
	{ "No supported compression algorithms have been found.", GNUTLS_E_NO_COMPRESSION_ALGORITHMS, 1 }, 
	{ "No supported cipher suites have been found.", GNUTLS_E_NO_CIPHER_SUITES, 1 }, 
	{0}
};

#define GNUTLS_ERROR_LOOP(b) \
        gnutls_error_entry *p; \
                for(p = error_algorithms; p->name != NULL; p++) { b ; }

#define GNUTLS_ERROR_ALG_LOOP(a) \
                        GNUTLS_ERROR_LOOP( if(p->number == error) { a; break; } )



/**
  * gnutls_error_is_fatal - Returns non-zero in case of a fatal error
  * @error: is an error returned by a gnutls function. Error should be a negative value.
  *
  * If a function returns a negative value you may feed that value
  * to this function to see if it is fatal. Returns 1 for a fatal 
  * error 0 otherwise. However you may want to check the
  * error code manualy, since some non-fatal errors to the protocol
  * may be fatal for you (your program).
  **/
int gnutls_error_is_fatal(int error)
{
	int ret = 0;

	GNUTLS_ERROR_ALG_LOOP(ret = p->fatal);
	return ret;
}

/**
  * gnutls_perror - prints a string to stderr with a description of an error
  * @error: is an error returned by a gnutls function. Error is always a negative value.
  *
  * This function is like perror(). The only difference is that it accepts an 
  * error returned by a gnutls function. 
  **/
void gnutls_perror(int error)
{
	char *ret = NULL;

	/* avoid prefix */
	GNUTLS_ERROR_ALG_LOOP(ret =
			      gnutls_strdup(p->name + sizeof("GNUTLS_E_") - 1));

	fprintf(stderr,  "GNUTLS ERROR: %s\n", ret);
	
	gnutls_free( ret);
}


/**
  * gnutls_strerror - Returns a string with a description of an error
  * @error: is an error returned by a gnutls function. Error is always a negative value.
  *
  * This function is similar to strerror(). The only difference is that it 
  * accepts an error (number) returned by a gnutls function. 
  **/
const char* gnutls_strerror(int error)
{
	const char *ret = NULL;

	/* avoid prefix */
	GNUTLS_ERROR_ALG_LOOP(ret =
			      p->name + sizeof("GNUTLS_E_") - 1);

	return ret;
}

int _gnutls_asn2err( int asn_err) {
	switch( asn_err) {
		case ASN1_FILE_NOT_FOUND:
			return GNUTLS_E_FILE_ERROR;
		case ASN1_ELEMENT_NOT_FOUND:
			return GNUTLS_E_ASN1_ELEMENT_NOT_FOUND;
		case ASN1_IDENTIFIER_NOT_FOUND:
			return GNUTLS_E_ASN1_IDENTIFIER_NOT_FOUND;
		case ASN1_DER_ERROR:
			return GNUTLS_E_ASN1_DER_ERROR;
		case ASN1_VALUE_NOT_FOUND:
			return GNUTLS_E_ASN1_VALUE_NOT_FOUND;
		case ASN1_GENERIC_ERROR:
			return GNUTLS_E_ASN1_GENERIC_ERROR;
		case ASN1_VALUE_NOT_VALID:
			return GNUTLS_E_ASN1_VALUE_NOT_VALID;
		case ASN1_TAG_ERROR:
			return GNUTLS_E_ASN1_TAG_ERROR;
		case ASN1_TAG_IMPLICIT:
			return GNUTLS_E_ASN1_TAG_IMPLICIT;
		case ASN1_ERROR_TYPE_ANY:
			return GNUTLS_E_ASN1_TYPE_ANY_ERROR;
		case ASN1_SYNTAX_ERROR:
			return GNUTLS_E_ASN1_SYNTAX_ERROR;
		case ASN1_MEM_ERROR:
			return GNUTLS_E_MEMORY_ERROR;
		case ASN1_DER_OVERFLOW:
			return GNUTLS_E_ASN1_DER_OVERFLOW;
		default:
			return GNUTLS_E_ASN1_GENERIC_ERROR;
	}
}


/* this function will output a message using the
 * caller provided function 
 */
#ifdef DEBUG
void _gnutls_log( const char *fmt, ...) {
 va_list args;
 char str[MAX_LOG_SIZE];
 void (*log_func)(const char*) = _gnutls_log_func;

 if (_gnutls_log_func==NULL) return;

 va_start(args,fmt);
 vsprintf( str,fmt,args); /* Flawfinder: ignore */
 va_end(args);   

 log_func( str);

 return;
}
#else /* not DEBUG */
# ifndef C99_MACROS

/* Without C99 macros these functions have to
 * be called. This may affect performance.
 */
void _gnutls_null_log( void* x, ...) { return; }
char* GET_CN( gnutls_datum x) { return NULL; }
const char* _gnutls_handshake2str( int handshake) { return NULL; }
char * _gnutls_bin2hex(const unsigned char *old, const size_t oldlen)
	{ return NULL; }
const char* _gnutls_packet2str( int packet) { return NULL; }

# endif /* C99_MACROS */
#endif /* DEBUG */
