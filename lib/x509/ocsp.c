/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
 * Author: Simon Josefsson
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

/* Online Certificate Status Protocol - RFC 2560
 */

#include <gnutls_int.h>
#include <gnutls_global.h>
#include <gnutls_errors.h>
#include <libtasn1.h>
#include <gnutls_pk.h>
#include "common.h"
#include "hash.h"
#include "verify-high.h"

#include <gnutls/ocsp.h>

typedef struct gnutls_ocsp_req_int
{
  ASN1_TYPE req;
} gnutls_ocsp_req_int;

typedef struct gnutls_ocsp_resp_int
{
  ASN1_TYPE resp;
  gnutls_datum_t response_type_oid;
  ASN1_TYPE basicresp;
} gnutls_ocsp_resp_int;

#define MAX_TIME 64

/**
 * gnutls_ocsp_req_init:
 * @req: The structure to be initialized
 *
 * This function will initialize an OCSP request structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_ocsp_req_init (gnutls_ocsp_req_t * req)
{
  gnutls_ocsp_req_t tmp = gnutls_calloc (1, sizeof (gnutls_ocsp_req_int));
  int ret;

  if (!tmp)
    return GNUTLS_E_MEMORY_ERROR;

  ret = asn1_create_element (_gnutls_get_pkix (), "PKIX1.OCSPRequest",
			     &tmp->req);
  if (ret != ASN1_SUCCESS)
    {
      gnutls_assert ();
      gnutls_free (tmp);
      return _gnutls_asn2err (ret);
    }

  *req = tmp;

  return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_ocsp_req_deinit:
 * @req: The structure to be deinitialized
 *
 * This function will deinitialize a OCSP request structure.
 **/
void
gnutls_ocsp_req_deinit (gnutls_ocsp_req_t req)
{
  if (!req)
    return;

  if (req->req)
    asn1_delete_structure (&req->req);

  req->req = NULL;

  gnutls_free (req);
}

/**
 * gnutls_ocsp_resp_init:
 * @resp: The structure to be initialized
 *
 * This function will initialize an OCSP response structure.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_ocsp_resp_init (gnutls_ocsp_resp_t * resp)
{
  gnutls_ocsp_resp_t tmp = gnutls_calloc (1, sizeof (gnutls_ocsp_resp_int));
  int ret;

  if (!tmp)
    return GNUTLS_E_MEMORY_ERROR;

  ret = asn1_create_element (_gnutls_get_pkix (),
				"PKIX1.OCSPResponse", &tmp->resp);
  if (ret != ASN1_SUCCESS)
    {
      gnutls_assert ();
      gnutls_free (tmp);
      return _gnutls_asn2err (ret);
    }

  ret = asn1_create_element (_gnutls_get_pkix (),
			     "PKIX1.BasicOCSPResponse", &tmp->basicresp);
  if (ret != ASN1_SUCCESS)
    {
      gnutls_assert ();
      asn1_delete_structure (&tmp->resp);
      gnutls_free (tmp);
      return _gnutls_asn2err (ret);
    }

  *resp = tmp;

  return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_ocsp_resp_deinit:
 * @resp: The structure to be deinitialized
 *
 * This function will deinitialize a OCSP response structure.
 **/
void
gnutls_ocsp_resp_deinit (gnutls_ocsp_resp_t resp)
{
  if (!resp)
    return;

  if (resp->resp)
    asn1_delete_structure (&resp->resp);
  gnutls_free (resp->response_type_oid.data);
  if (resp->basicresp)
    asn1_delete_structure (&resp->basicresp);

  resp->resp = NULL;
  resp->response_type_oid.data = NULL;
  resp->basicresp = NULL;

  gnutls_free (resp);
}

/**
 * gnutls_ocsp_req_import:
 * @req: The structure to store the parsed request.
 * @data: DER encoded OCSP request.
 *
 * This function will convert the given DER encoded OCSP request to
 * the native #gnutls_ocsp_req_t format. The output will be stored in
 * @req.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_ocsp_req_import (gnutls_ocsp_req_t req,
                        const gnutls_datum_t * data)
{
  int ret = 0;

  if (req == NULL || data == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  if (req->req)
    {
      /* Any earlier asn1_der_decoding will modify the ASN.1
         structure, so we need to replace it with a fresh
         structure. */
      asn1_delete_structure (&req->req);

      ret = asn1_create_element (_gnutls_get_pkix (),
                                    "PKIX1.OCSPRequest", &req->req);
      if (ret != ASN1_SUCCESS)
        {
          gnutls_assert ();
          return _gnutls_asn2err (ret);
        }
    }

  ret = asn1_der_decoding (&req->req, data->data, data->size, NULL);
  if (ret != ASN1_SUCCESS)
    {
      gnutls_assert ();
      return _gnutls_asn2err (ret);
    }

  return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_ocsp_resp_import:
 * @resp: The structure to store the parsed response.
 * @data: DER encoded OCSP response.
 *
 * This function will convert the given DER encoded OCSP response to
 * the native #gnutls_ocsp_resp_t format.  It also decodes the Basic
 * OCSP Response part, if any.  The output will be stored in @resp.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_ocsp_resp_import (gnutls_ocsp_resp_t resp,
			 const gnutls_datum_t * data)
{
  int ret = 0;

  if (resp == NULL || data == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  if (resp->resp)
    {
      /* Any earlier asn1_der_decoding will modify the ASN.1
         structure, so we need to replace it with a fresh
         structure. */
      asn1_delete_structure (&resp->resp);

      ret = asn1_create_element (_gnutls_get_pkix (),
				 "PKIX1.OCSPResponse", &resp->resp);
      if (ret != ASN1_SUCCESS)
        {
          gnutls_assert ();
          return _gnutls_asn2err (ret);
        }
    }

  ret = asn1_der_decoding (&resp->resp, data->data, data->size, NULL);
  if (ret != ASN1_SUCCESS)
    {
      gnutls_assert ();
      return _gnutls_asn2err (ret);
    }

  if (gnutls_ocsp_resp_get_status (resp) != GNUTLS_OCSP_RESP_SUCCESSFUL)
    return GNUTLS_E_SUCCESS;

  ret = _gnutls_x509_read_value (resp->resp, "responseBytes.responseType",
				 &resp->response_type_oid, 0);
  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

#define OCSP_BASIC "1.3.6.1.5.5.7.48.1.1"

  if (resp->response_type_oid.size == sizeof (OCSP_BASIC)
      && memcmp (resp->response_type_oid.data, OCSP_BASIC,
		 resp->response_type_oid.size) == 0)
    {
      gnutls_datum_t d;

      if (resp->basicresp)
	{
	  asn1_delete_structure (&resp->basicresp);

	  ret = asn1_create_element (_gnutls_get_pkix (),
				     "PKIX1.BasicOCSPResponse", &resp->basicresp);
	  if (ret != ASN1_SUCCESS)
	    {
	      gnutls_assert ();
	      return _gnutls_asn2err (ret);
	    }
	}

      ret = _gnutls_x509_read_value (resp->resp, "responseBytes.response",
				     &d, 0);
      if (ret < 0)
	{
	  gnutls_assert ();
	  return ret;
	}

      ret = asn1_der_decoding (&resp->basicresp, d.data, d.size, NULL);
      gnutls_free (d.data);
      if (ret != ASN1_SUCCESS)
	{
	  gnutls_assert ();
	  return _gnutls_asn2err (ret);
	}
    }
  else
    resp->basicresp = NULL;

  return GNUTLS_E_SUCCESS;
}

static int
export (ASN1_TYPE node, const char *name, gnutls_datum_t * data)
{
  int ret;
  int len = 0;

  ret = asn1_der_coding (node, name, NULL, &len, NULL);
  if (ret != ASN1_MEM_ERROR)
    {
      gnutls_assert ();
      return _gnutls_asn2err (ret);
    }
  data->size = len;
  data->data = gnutls_malloc (len);
  if (data->data == NULL)
    return GNUTLS_E_MEMORY_ERROR;
  ret = asn1_der_coding (node, name, data->data, &len, NULL);
  if (ret != ASN1_SUCCESS)
    {
      gnutls_assert ();
      return _gnutls_asn2err (ret);
    }

  return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_ocsp_req_export:
 * @req: Holds the OCSP request
 * @data: newly allocate buffer holding DER encoded OCSP request
 *
 * This function will export the OCSP request to DER format.
 *
 * Returns: In case of failure a negative error code will be
 *   returned, and 0 on success.
 **/
int
gnutls_ocsp_req_export (gnutls_ocsp_req_t req, gnutls_datum_t * data)
{
  int ret;

  if (req == NULL || data == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  /* XXX remove when we support these fields */
  asn1_write_value (req->req, "tbsRequest.requestorName", NULL, 0);
  asn1_write_value (req->req, "optionalSignature", NULL, 0);

  /* prune extension field if we don't have any extension */
  ret = gnutls_ocsp_req_get_extension (req, 0, NULL, NULL, NULL);
  if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
    asn1_write_value (req->req, "tbsRequest.requestExtensions", NULL, 0);

  return export (req->req, "", data);
}

/**
 * gnutls_ocsp_resp_export:
 * @resp: Holds the OCSP response
 * @data: newly allocate buffer holding DER encoded OCSP response
 *
 * This function will export the OCSP response to DER format.
 *
 * Returns: In case of failure a negative error code will be
 *   returned, and 0 on success.
 **/
int
gnutls_ocsp_resp_export (gnutls_ocsp_resp_t resp, gnutls_datum_t * data)
{
  if (resp == NULL || data == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  return export (resp->resp, "", data);
}

/**
 * gnutls_ocsp_req_get_version:
 * @req: should contain a #gnutls_ocsp_req_t structure
 *
 * This function will return the version of the OCSP request.
 * Typically this is always 1 indicating version 1.
 *
 * Returns: version of OCSP request, or a negative error code on error.
 **/
int
gnutls_ocsp_req_get_version (gnutls_ocsp_req_t req)
{
  uint8_t version[8];
  int len, ret;

  if (req == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  len = sizeof (version);
  ret = asn1_read_value (req->req, "tbsRequest.version", version, &len);
  if (ret != ASN1_SUCCESS)
    {
      if (ret == ASN1_ELEMENT_NOT_FOUND)
        return 1;               /* the DEFAULT version */
      gnutls_assert ();
      return _gnutls_asn2err (ret);
    }

  return (int) version[0] + 1;
}

/**
 * gnutls_ocsp_req_get_cert_id:
 * @req: should contain a #gnutls_ocsp_req_t structure
 * @indx: Specifies which extension OID to get. Use (0) to get the first one.
 * @digest: output variable with #gnutls_digest_algorithm_t hash algorithm
 * @issuer_name_hash: output buffer with hash of issuer's DN
 * @issuer_key_hash: output buffer with hash of issuer's public key
 * @serial_number: output buffer with serial number of certificate to check
 *
 * This function will return the certificate information of the
 * @indx'ed request in the OCSP request.  The information returned
 * corresponds to the CertID structure:
 *
 * <informalexample><programlisting>
 *    CertID          ::=     SEQUENCE {
 *        hashAlgorithm       AlgorithmIdentifier,
 *        issuerNameHash      OCTET STRING, -- Hash of Issuer's DN
 *        issuerKeyHash       OCTET STRING, -- Hash of Issuers public key
 *        serialNumber        CertificateSerialNumber }
 * </programlisting></informalexample>
 *
 * Each of the pointers to output variables may be NULL to indicate
 * that the caller is not interested in that value.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code is returned.  If you have reached the last
 *   CertID available %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will be
 *   returned.
 **/
int
gnutls_ocsp_req_get_cert_id (gnutls_ocsp_req_t req,
			     unsigned indx,
			     gnutls_digest_algorithm_t *digest,
			     gnutls_datum_t *issuer_name_hash,
			     gnutls_datum_t *issuer_key_hash,
			     gnutls_datum_t *serial_number)
{
  gnutls_datum_t sa;
  char name[ASN1_MAX_NAME_SIZE];
  int ret;

  if (req == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  snprintf (name, sizeof (name),
	    "tbsRequest.requestList.?%u.reqCert.hashAlgorithm.algorithm",
	    indx + 1);
  ret = _gnutls_x509_read_value (req->req, name, &sa, 0);
  if (ret == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND)
    return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
  else if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  ret = _gnutls_x509_oid2digest_algorithm ((char*)sa.data);
  _gnutls_free_datum (&sa);
  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  if (digest)
    *digest = ret;

  if (issuer_name_hash)
    {
      snprintf (name, sizeof (name),
		"tbsRequest.requestList.?%u.reqCert.issuerNameHash", indx + 1);
      ret = _gnutls_x509_read_value (req->req, name, issuer_name_hash, 0);
      if (ret != GNUTLS_E_SUCCESS)
	{
	  gnutls_assert ();
	  return ret;
	}
    }

  if (issuer_key_hash)
    {
      snprintf (name, sizeof (name),
		"tbsRequest.requestList.?%u.reqCert.issuerKeyHash", indx + 1);
      ret = _gnutls_x509_read_value (req->req, name, issuer_key_hash, 0);
      if (ret != GNUTLS_E_SUCCESS)
	{
	  gnutls_assert ();
	  if (issuer_name_hash)
	    gnutls_free (issuer_name_hash->data);
	  return ret;
	}
    }

  if (serial_number)
    {
      snprintf (name, sizeof (name),
		"tbsRequest.requestList.?%u.reqCert.serialNumber", indx + 1);
      ret = _gnutls_x509_read_value (req->req, name, serial_number, 0);
      if (ret != GNUTLS_E_SUCCESS)
	{
	  gnutls_assert ();
	  if (issuer_name_hash)
	    gnutls_free (issuer_name_hash->data);
	  if (issuer_key_hash)
	    gnutls_free (issuer_key_hash->data);
	  return ret;
	}
    }

  return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_ocsp_req_add_cert_id:
 * @req: should contain a #gnutls_ocsp_req_t structure
 * @digest: hash algorithm, a #gnutls_digest_algorithm_t value
 * @issuer_name_hash: hash of issuer's DN
 * @issuer_key_hash: hash of issuer's public key
 * @serial_number: serial number of certificate to check
 *
 * This function will add another request to the OCSP request for a
 * particular certificate having the issuer name hash of
 * @issuer_name_hash and issuer key hash of @issuer_key_hash (both
 * hashed using @digest) and serial number @serial_number.
 *
 * The information needed corresponds to the CertID structure:
 *
 * <informalexample><programlisting>
 *    CertID          ::=     SEQUENCE {
 *        hashAlgorithm       AlgorithmIdentifier,
 *        issuerNameHash      OCTET STRING, -- Hash of Issuer's DN
 *        issuerKeyHash       OCTET STRING, -- Hash of Issuers public key
 *        serialNumber        CertificateSerialNumber }
 * </programlisting></informalexample>
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code is returned.
 **/
int
gnutls_ocsp_req_add_cert_id (gnutls_ocsp_req_t req,
			     gnutls_digest_algorithm_t digest,
			     const gnutls_datum_t *issuer_name_hash,
			     const gnutls_datum_t *issuer_key_hash,
			     const gnutls_datum_t *serial_number)
{
  int result;
  const char *oid;

  if (req == NULL || issuer_name_hash == NULL
      || issuer_key_hash == NULL || serial_number == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  oid = _gnutls_x509_digest_to_oid (digest);
  if (oid == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  result = asn1_write_value (req->req, "tbsRequest.requestList", "NEW", 1);
  if (result != ASN1_SUCCESS)
    {
      gnutls_assert ();
      return _gnutls_asn2err (result);
    }

  result = asn1_write_value
    (req->req, "tbsRequest.requestList.?LAST.reqCert.hashAlgorithm.algorithm",
     oid, 1);
  if (result != ASN1_SUCCESS)
    {
      gnutls_assert ();
      return _gnutls_asn2err (result);
    }

  /* XXX we don't support any algorithm with parameters */
  result = asn1_write_value
    (req->req, "tbsRequest.requestList.?LAST.reqCert.hashAlgorithm.parameters",
     ASN1_NULL, ASN1_NULL_SIZE);
  if (result != ASN1_SUCCESS)
    {
      gnutls_assert ();
      return _gnutls_asn2err (result);
    }

  result = asn1_write_value
    (req->req, "tbsRequest.requestList.?LAST.reqCert.issuerNameHash",
     issuer_name_hash->data, issuer_name_hash->size);
  if (result != ASN1_SUCCESS)
    {
      gnutls_assert ();
      return _gnutls_asn2err (result);
    }

  result = asn1_write_value
    (req->req, "tbsRequest.requestList.?LAST.reqCert.issuerKeyHash",
     issuer_key_hash->data, issuer_key_hash->size);
  if (result != ASN1_SUCCESS)
    {
      gnutls_assert ();
      return _gnutls_asn2err (result);
    }

  result = asn1_write_value
    (req->req, "tbsRequest.requestList.?LAST.reqCert.serialNumber",
     serial_number->data, serial_number->size);
  if (result != ASN1_SUCCESS)
    {
      gnutls_assert ();
      return _gnutls_asn2err (result);
    }

  /* XXX add separate function that can add extensions too */
  result = asn1_write_value
    (req->req, "tbsRequest.requestList.?LAST.singleRequestExtensions",
     NULL, 0);
  if (result != ASN1_SUCCESS)
    {
      gnutls_assert ();
      return _gnutls_asn2err (result);
    }

  return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_ocsp_req_add_cert:
 * @req: should contain a #gnutls_ocsp_req_t structure
 * @digest: hash algorithm, a #gnutls_digest_algorithm_t value
 * @issuer: issuer of @subject certificate
 * @cert: certificate to request status for
 *
 * This function will add another request to the OCSP request for a
 * particular certificate.  The issuer name hash, issuer key hash, and
 * serial number fields is populated as follows.  The issuer name and
 * the serial number is taken from @cert.  The issuer key is taken
 * from @issuer.  The hashed values will be hashed using the @digest
 * algorithm, normally %GNUTLS_DIG_SHA1.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code is returned.
 **/
int
gnutls_ocsp_req_add_cert (gnutls_ocsp_req_t req,
			  gnutls_digest_algorithm_t digest,
			  gnutls_x509_crt_t issuer,
			  gnutls_x509_crt_t cert)
{
  int ret;
  gnutls_datum_t sn, tmp, inh, ikh;
  uint8_t inh_buf[MAX_HASH_SIZE];
  uint8_t ikh_buf[MAX_HASH_SIZE];
  size_t inhlen = MAX_HASH_SIZE;
  size_t ikhlen = MAX_HASH_SIZE;

  if (req == NULL || issuer == NULL || cert == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  ret = _gnutls_x509_der_encode (cert->cert,
				 "tbsCertificate.issuer.rdnSequence",
				 &tmp, 0);
  if (ret != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      return ret;
    }

  ret = gnutls_fingerprint (digest, &tmp, inh_buf, &inhlen);
  gnutls_free (tmp.data);
  if (ret != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      return ret;
    }
  inh.size = inhlen;
  inh.data = inh_buf;

  ret = _gnutls_x509_read_value
    (issuer->cert, "tbsCertificate.subjectPublicKeyInfo.subjectPublicKey",
     &tmp, 2);
  if (ret != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      return ret;
    }

  ret = gnutls_fingerprint (digest, &tmp, ikh_buf, &ikhlen);
  gnutls_free (tmp.data);
  if (ret != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      return ret;
    }
  ikh.size = ikhlen;
  ikh.data = ikh_buf;

  ret = _gnutls_x509_read_value (cert->cert, "tbsCertificate.serialNumber",
				 &sn, 0);
  if (ret != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      return ret;
    }

  ret = gnutls_ocsp_req_add_cert_id (req, digest, &inh, &ikh, &sn);
  gnutls_free (sn.data);
  if (ret != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      return ret;
    }

  return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_ocsp_req_get_extension:
 * @req: should contain a #gnutls_ocsp_req_t structure
 * @indx: Specifies which extension OID to get. Use (0) to get the first one.
 * @oid: will hold newly allocated buffer with OID of extension, may be NULL
 * @critical: output variable with critical flag, may be NULL.
 * @data: will hold newly allocated buffer with extension data, may be NULL
 *
 * This function will return all information about the requested
 * extension in the OCSP request.  The information returned is the
 * OID, the critical flag, and the data itself.  The extension OID
 * will be stored as a string.  Any of @oid, @critical, and @data may
 * be NULL which means that the caller is not interested in getting
 * that information back.
 *
 * The caller needs to deallocate memory by calling gnutls_free() on
 * @oid->data and @data->data.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code is returned.  If you have reached the last
 *   extension available %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will
 *   be returned.
 **/
int
gnutls_ocsp_req_get_extension (gnutls_ocsp_req_t req,
			       unsigned indx,
			       gnutls_datum_t *oid,
			       unsigned int *critical,
			       gnutls_datum_t *data)
{
  int ret;
  char str_critical[10];
  char name[ASN1_MAX_NAME_SIZE];
  int len;

  if (!req)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  snprintf (name, sizeof (name), "tbsRequest.requestExtensions.?%u.critical",
	    indx + 1);
  len = sizeof (str_critical);
  ret = asn1_read_value (req->req, name, str_critical, &len);
  if (ret == ASN1_ELEMENT_NOT_FOUND)
    return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
  else if (ret != ASN1_SUCCESS)
    {
      gnutls_assert ();
      return _gnutls_asn2err (ret);
    }

  if (critical)
    {
      if (str_critical[0] == 'T')
	*critical = 1;
      else
	*critical = 0;
    }

  if (oid)
    {
      snprintf (name, sizeof (name),
		"tbsRequest.requestExtensions.?%u.extnID", indx + 1);
      ret = _gnutls_x509_read_value (req->req, name, oid, 0);
      if (ret != GNUTLS_E_SUCCESS)
	{
	  gnutls_assert ();
	  return ret;
	}
    }

  if (data)
    {
      snprintf (name, sizeof (name),
		"tbsRequest.requestExtensions.?%u.extnValue", indx + 1);
      ret = _gnutls_x509_read_value (req->req, name, data, 0);
      if (ret != GNUTLS_E_SUCCESS)
	{
	  gnutls_assert ();
	  if (oid)
	    gnutls_free (oid->data);
	  return ret;
	}
    }

  return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_ocsp_req_set_extension:
 * @req: should contain a #gnutls_ocsp_req_t structure
 * @oid: buffer with OID of extension as a string.
 * @critical: critical flag, normally false.
 * @data: the extension data
 *
 * This function will add an extension to the OCSP request.  Calling
 * this function multiple times for the same OID will overwrite values
 * from earlier calls.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code is returned.
 **/
int
gnutls_ocsp_req_set_extension (gnutls_ocsp_req_t req,
			       const char *oid,
			       unsigned int critical,
			       const gnutls_datum_t *data)
{
  if (req == NULL || oid == NULL || data == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  return set_extension (req->req, "tbsRequest.requestExtensions", oid,
			data, critical);
}

/**
 * gnutls_ocsp_req_get_nonce:
 * @req: should contain a #gnutls_ocsp_req_t structure
 * @critical: whether nonce extension is marked critical, or NULL
 * @nonce: will hold newly allocated buffer with nonce data
 *
 * This function will return the OCSP request nonce extension data.
 *
 * The caller needs to deallocate memory by calling gnutls_free() on
 * @nonce->data.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code is returned.
 **/
int
gnutls_ocsp_req_get_nonce (gnutls_ocsp_req_t req,
			   unsigned int *critical,
			   gnutls_datum_t *nonce)
{
  int ret;
  size_t l = 0;
  gnutls_datum_t tmp;

  if (req == NULL || nonce == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  ret = get_extension (req->req, "tbsRequest.requestExtensions",
		       GNUTLS_OCSP_NONCE, 0,
		       &tmp, critical);
  if (ret != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      return ret;
    }

  ret = _gnutls_x509_decode_octet_string (NULL, tmp.data, (size_t) tmp.size,
					  NULL, &l);
  if (ret != GNUTLS_E_SHORT_MEMORY_BUFFER)
    {
      gnutls_assert ();
      gnutls_free (tmp.data);
      return ret;
    }

  nonce->data = gnutls_malloc (l);
  if (nonce->data == NULL)
    {
      gnutls_assert ();
      gnutls_free (tmp.data);
      return GNUTLS_E_MEMORY_ERROR;
    }

  ret = _gnutls_x509_decode_octet_string (NULL, tmp.data, (size_t) tmp.size,
					  nonce->data, &l);
  gnutls_free (tmp.data);
  if (ret != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      return ret;
    }
  nonce->size = l;

  return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_ocsp_req_set_nonce:
 * @req: should contain a #gnutls_ocsp_req_t structure
 * @critical: critical flag, normally false.
 * @nonce: the nonce data
 *
 * This function will add an nonce extension to the OCSP request.
 * Calling this function multiple times will overwrite values from
 * earlier calls.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code is returned.
 **/
int
gnutls_ocsp_req_set_nonce (gnutls_ocsp_req_t req,
			   unsigned int critical,
			   const gnutls_datum_t *nonce)
{
  int ret;
  gnutls_datum_t dernonce;
  unsigned char temp[SIZEOF_UNSIGNED_LONG_INT + 1];
  int len;

  if (req == NULL || nonce == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  asn1_length_der (nonce->size, temp, &len);

  dernonce.size = 1 + len + nonce->size;
  dernonce.data = gnutls_malloc (dernonce.size);
  if (dernonce.data == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  dernonce.data[0] = '\x04';
  memcpy (dernonce.data + 1, temp, len);
  memcpy (dernonce.data + 1 + len, nonce->data, nonce->size);

  ret =  set_extension (req->req, "tbsRequest.requestExtensions",
			GNUTLS_OCSP_NONCE, &dernonce, critical);
  gnutls_free (dernonce.data);
  if (ret != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      return ret;
    }

  return ret;
}

/**
 * gnutls_ocsp_req_randomize_nonce:
 * @req: should contain a #gnutls_ocsp_req_t structure
 *
 * This function will add or update an nonce extension to the OCSP
 * request with a newly generated random value.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code is returned.
 **/
int
gnutls_ocsp_req_randomize_nonce (gnutls_ocsp_req_t req)
{
  int ret;
  uint8_t rndbuf[23];
  gnutls_datum_t nonce = { rndbuf, sizeof (rndbuf) };

  if (req == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  ret = gnutls_rnd (GNUTLS_RND_NONCE, rndbuf, sizeof (rndbuf));
  if (ret != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      return ret;
    }

  ret = gnutls_ocsp_req_set_nonce (req, 0, &nonce);
  if (ret != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      return ret;
    }

  return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_ocsp_resp_get_status:
 * @resp: should contain a #gnutls_ocsp_resp_t structure
 *
 * This function will return the status of a OCSP response, an
 * #gnutls_ocsp_resp_status_t enumeration.
 *
 * Returns: status of OCSP request as a #gnutls_ocsp_resp_status_t, or
 *   a negative error code on error.
 **/
int
gnutls_ocsp_resp_get_status (gnutls_ocsp_resp_t resp)
{
  uint8_t str[1];
  int len, ret;

  if (resp == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  len = sizeof (str);
  ret = asn1_read_value (resp->resp, "responseStatus", str, &len);
  if (ret != ASN1_SUCCESS)
    {
      gnutls_assert ();
      return _gnutls_asn2err (ret);
    }

  switch (str[0])
    {
    case GNUTLS_OCSP_RESP_SUCCESSFUL:
    case GNUTLS_OCSP_RESP_MALFORMEDREQUEST:
    case GNUTLS_OCSP_RESP_INTERNALERROR:
    case GNUTLS_OCSP_RESP_TRYLATER:
    case GNUTLS_OCSP_RESP_SIGREQUIRED:
    case GNUTLS_OCSP_RESP_UNAUTHORIZED:
      break;
    default:
      return GNUTLS_E_UNEXPECTED_PACKET;
    }

  return (int) str[0];
}

/**
 * gnutls_ocsp_resp_get_response:
 * @resp: should contain a #gnutls_ocsp_resp_t structure
 * @response_type_oid: newly allocated output buffer with response type OID
 * @response: newly allocated output buffer with DER encoded response
 *
 * This function will extract the response type OID in and the
 * response data from an OCSP response.  Normally the
 * @response_type_oid is always "1.3.6.1.5.5.7.48.1.1" which means the
 * @response should be decoded as a Basic OCSP Response, but
 * technically other response types could be used.
 *
 * This function is typically only useful when you want to extract the
 * response type OID of an response for diagnostic purposes.
 * Otherwise gnutls_ocsp_resp_import() will decode the basic OCSP
 * response part and the caller need not worry about that aspect.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_ocsp_resp_get_response (gnutls_ocsp_resp_t resp,
			       gnutls_datum_t *response_type_oid,
			       gnutls_datum_t *response)
{
  int ret;

  if (resp == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  if (response_type_oid != NULL)
    {
      ret = _gnutls_x509_read_value (resp->resp, "responseBytes.responseType",
				     response_type_oid, 0);
      if (ret < 0)
	{
	  gnutls_assert ();
	  return ret;
	}
    }

  if (response != NULL)
    {
      ret = _gnutls_x509_read_value (resp->resp, "responseBytes.response",
				     response, 0);
      if (ret < 0)
	{
	  gnutls_assert ();
	  return ret;
	}
    }

  return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_ocsp_resp_get_version:
 * @resp: should contain a #gnutls_ocsp_resp_t structure
 *
 * This function will return the version of the Basic OCSP Response.
 * Typically this is always 1 indicating version 1.
 *
 * Returns: version of Basic OCSP response, or a negative error code
 *   on error.
 **/
int
gnutls_ocsp_resp_get_version (gnutls_ocsp_resp_t resp)
{
  uint8_t version[8];
  int len, ret;

  if (resp == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  len = sizeof (version);
  ret = asn1_read_value (resp->resp, "tbsResponseData.version", version, &len);
  if (ret != ASN1_SUCCESS)
    {
      if (ret == ASN1_ELEMENT_NOT_FOUND)
        return 1;               /* the DEFAULT version */
      gnutls_assert ();
      return _gnutls_asn2err (ret);
    }

  return (int) version[0] + 1;
}

/**
 * gnutls_ocsp_resp_get_responder:
 * @resp: should contain a #gnutls_ocsp_resp_t structure
 * @dn: newly allocated buffer with name
 *
 * This function will extract the name of the Basic OCSP Response in
 * the provided buffer. The name will be in the form
 * "C=xxxx,O=yyyy,CN=zzzz" as described in RFC2253. The output string
 * will be ASCII or UTF-8 encoded, depending on the certificate data.
 *
 * The caller needs to deallocate memory by calling gnutls_free() on
 * @dn->data.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code is returned.
 **/
int
gnutls_ocsp_resp_get_responder (gnutls_ocsp_resp_t resp,
				gnutls_datum_t *dn)
{
  int ret;
  size_t l = 0;

  if (resp == NULL || dn == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  ret =  _gnutls_x509_parse_dn
    (resp->basicresp, "tbsResponseData.responderID.byName",
     NULL, &l);
  if (ret != GNUTLS_E_SHORT_MEMORY_BUFFER)
    {
      gnutls_assert ();
      return ret;
    }

  dn->data = gnutls_malloc (l);
  if (dn->data == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  ret =  _gnutls_x509_parse_dn
    (resp->basicresp, "tbsResponseData.responderID.byName",
     (char*)dn->data, &l);
  if (ret != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      return ret;
    }

  dn->size = l;

  return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_ocsp_resp_get_produced:
 * @resp: should contain a #gnutls_ocsp_resp_t structure
 *
 * This function will return the time when the OCSP response was
 * signed.
 *
 * Returns: signing time, or (time_t)-1 on error.
 **/
time_t
gnutls_ocsp_resp_get_produced (gnutls_ocsp_resp_t resp)
{
  char ttime[MAX_TIME];
  int len, ret;
  time_t c_time;

  if (resp == NULL || resp->basicresp == NULL)
    {
      gnutls_assert ();
      return (time_t) (-1);
    }

  len = sizeof (ttime) - 1;
  ret = asn1_read_value (resp->basicresp, "tbsResponseData.producedAt",
			 ttime, &len);
  if (ret != ASN1_SUCCESS)
    {
      gnutls_assert ();
      return (time_t) (-1);
    }

  c_time = _gnutls_x509_generalTime2gtime (ttime);

  return c_time;
}

/**
 * gnutls_ocsp_resp_get_single:
 * @resp: should contain a #gnutls_ocsp_resp_t structure
 * @indx: Specifies which extension OID to get. Use (0) to get the first one.
 * @digest: output variable with #gnutls_digest_algorithm_t hash algorithm
 * @issuer_name_hash: output buffer with hash of issuer's DN
 * @issuer_key_hash: output buffer with hash of issuer's public key
 * @serial_number: output buffer with serial number of certificate to check
 * @cert_status: a certificate status, a #gnutls_ocsp_cert_status_t enum.
 * @this_update: time at which the status is known to be correct.
 * @next_update: when newer information will be available, or (time_t)-1 if unspecified
 * @revocation_time: when @cert_status is %GNUTLS_OCSP_CERT_REVOKED, holds time of revocation.
 * @revocation_reason: revocation reason, a #gnutls_x509_crl_reason_t enum.
 *
 * This function will return the certificate information of the
 * @indx'ed response in the Basic OCSP Response @resp.  The
 * information returned corresponds to the SingleResponse structure
 * except the final singleExtensions, reproduced here for illustration:
 *
 * <informalexample><programlisting>
 * SingleResponse ::= SEQUENCE {
 *    certID                       CertID,
 *    certStatus                   CertStatus,
 *    thisUpdate                   GeneralizedTime,
 *    nextUpdate         [0]       EXPLICIT GeneralizedTime OPTIONAL,
 *    singleExtensions   [1]       EXPLICIT Extensions OPTIONAL }
 *
 *    CertID          ::=     SEQUENCE {
 *        hashAlgorithm       AlgorithmIdentifier,
 *        issuerNameHash      OCTET STRING, -- Hash of Issuer's DN
 *        issuerKeyHash       OCTET STRING, -- Hash of Issuers public key
 *        serialNumber        CertificateSerialNumber }
 *
 * CertStatus ::= CHOICE {
 *     good                [0]     IMPLICIT NULL,
 *     revoked             [1]     IMPLICIT RevokedInfo,
 *     unknown             [2]     IMPLICIT UnknownInfo }
 * 
 * RevokedInfo ::= SEQUENCE {
 *     revocationTime              GeneralizedTime,
 *     revocationReason    [0]     EXPLICIT CRLReason OPTIONAL }
 * </programlisting></informalexample>
 *
 * Each of the pointers to output variables may be NULL to indicate
 * that the caller is not interested in that value.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code is returned.  If you have reached the last
 *   CertID available %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will be
 *   returned.
 **/
int
gnutls_ocsp_resp_get_single (gnutls_ocsp_resp_t resp,
			     unsigned indx,
			     gnutls_digest_algorithm_t *digest,
			     gnutls_datum_t *issuer_name_hash,
			     gnutls_datum_t *issuer_key_hash,
			     gnutls_datum_t *serial_number,
			     int *cert_status,
			     time_t *this_update,
			     time_t *next_update,
			     time_t *revocation_time,
			     int *revocation_reason)
{
  gnutls_datum_t sa;
  char name[ASN1_MAX_NAME_SIZE];
  int ret;

  snprintf (name, sizeof (name),
	    "tbsResponseData.responses.?%u.certID.hashAlgorithm.algorithm",
	    indx + 1);
  ret = _gnutls_x509_read_value (resp->basicresp, name, &sa, 0);
  if (ret == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND)
    return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
  else if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  ret = _gnutls_x509_oid2digest_algorithm ((char*)sa.data);
  _gnutls_free_datum (&sa);
  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  if (digest)
    *digest = ret;

  if (issuer_name_hash)
    {
      snprintf (name, sizeof (name),
		"tbsResponseData.responses.?%u.certID.issuerNameHash",
		indx + 1);
      ret = _gnutls_x509_read_value (resp->basicresp, name,
				     issuer_name_hash, 0);
      if (ret != GNUTLS_E_SUCCESS)
	{
	  gnutls_assert ();
	  return ret;
	}
    }

  if (issuer_key_hash)
    {
      snprintf (name, sizeof (name),
		"tbsResponseData.responses.?%u.certID.issuerKeyHash",
		indx + 1);
      ret = _gnutls_x509_read_value (resp->basicresp, name,
				     issuer_key_hash, 0);
      if (ret != GNUTLS_E_SUCCESS)
	{
	  gnutls_assert ();
	  if (issuer_name_hash)
	    gnutls_free (issuer_name_hash->data);
	  return ret;
	}
    }

  if (serial_number)
    {
      snprintf (name, sizeof (name),
		"tbsResponseData.responses.?%u.certID.serialNumber",
		indx + 1);
      ret = _gnutls_x509_read_value (resp->basicresp, name,
				     serial_number, 0);
      if (ret != GNUTLS_E_SUCCESS)
	{
	  gnutls_assert ();
	  if (issuer_name_hash)
	    gnutls_free (issuer_name_hash->data);
	  if (issuer_key_hash)
	    gnutls_free (issuer_key_hash->data);
	  return ret;
	}
    }

  if (cert_status)
    {
      snprintf (name, sizeof (name),
		"tbsResponseData.responses.?%u.certStatus",
		indx + 1);
      ret = _gnutls_x509_read_value (resp->basicresp, name, &sa, 0);
      if (ret == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND)
	return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
      else if (ret < 0)
	{
	  gnutls_assert ();
	  return ret;
	}
      if (sa.size == 5 && memcmp (sa.data, "good", sa.size) == 0)
	*cert_status = GNUTLS_OCSP_CERT_GOOD;
      else if (sa.size == 8 && memcmp (sa.data, "revoked", sa.size) == 0)
	*cert_status = GNUTLS_OCSP_CERT_REVOKED;
      else if (sa.size == 8 && memcmp (sa.data, "unknown", sa.size) == 0)
	*cert_status = GNUTLS_OCSP_CERT_UNKNOWN;
      else
	{
	  gnutls_assert ();
	  gnutls_free (sa.data);
	  return GNUTLS_E_ASN1_DER_ERROR;
	}
      gnutls_free (sa.data);
    }

  if (this_update)
    {
      char ttime[MAX_TIME];
      int len;

      snprintf (name, sizeof (name),
		"tbsResponseData.responses.?%u.thisUpdate",
		indx + 1);
      len = sizeof (ttime) - 1;
      ret = asn1_read_value (resp->basicresp, name, ttime, &len);
      if (ret != ASN1_SUCCESS)
	{
	  gnutls_assert ();
	  *this_update = (time_t) (-1);
	}
      else
	*this_update = _gnutls_x509_generalTime2gtime (ttime);
    }

  if (next_update)
    {
      char ttime[MAX_TIME];
      int len;

      snprintf (name, sizeof (name),
		"tbsResponseData.responses.?%u.nextUpdate",
		indx + 1);
      len = sizeof (ttime) - 1;
      ret = asn1_read_value (resp->basicresp, name, ttime, &len);
      if (ret != ASN1_SUCCESS)
	{
	  gnutls_assert ();
	  *next_update = (time_t) (-1);
	}
      else
	*next_update = _gnutls_x509_generalTime2gtime (ttime);
    }

  if (revocation_time)
    {
      char ttime[MAX_TIME];
      int len;

      snprintf (name, sizeof (name),
		"tbsResponseData.responses.?%u.certStatus."
		"revoked.revocationTime",
		indx + 1);
      len = sizeof (ttime) - 1;
      ret = asn1_read_value (resp->basicresp, name, ttime, &len);
      if (ret != ASN1_SUCCESS)
	{
	  gnutls_assert ();
	  *revocation_time = (time_t) (-1);
	}
      else
	*revocation_time = _gnutls_x509_generalTime2gtime (ttime);
    }

  /* revocation_reason */

  return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_ocsp_resp_get_extension:
 * @resp: should contain a #gnutls_ocsp_resp_t structure
 * @indx: Specifies which extension OID to get. Use (0) to get the first one.
 * @oid: will hold newly allocated buffer with OID of extension, may be NULL
 * @critical: output variable with critical flag, may be NULL.
 * @data: will hold newly allocated buffer with extension data, may be NULL
 *
 * This function will return all information about the requested
 * extension in the OCSP response.  The information returned is the
 * OID, the critical flag, and the data itself.  The extension OID
 * will be stored as a string.  Any of @oid, @critical, and @data may
 * be NULL which means that the caller is not interested in getting
 * that information back.
 *
 * The caller needs to deallocate memory by calling gnutls_free() on
 * @oid->data and @data->data.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code is returned.  If you have reached the last
 *   extension available %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will
 *   be returned.
 **/
int
gnutls_ocsp_resp_get_extension (gnutls_ocsp_resp_t resp,
				unsigned indx,
				gnutls_datum_t *oid,
				unsigned int *critical,
				gnutls_datum_t *data)
{
  int ret;
  char str_critical[10];
  char name[ASN1_MAX_NAME_SIZE];
  int len;

  if (!resp)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  snprintf (name, sizeof (name),
	    "tbsResponseData.responseExtensions.?%u.critical",
	    indx + 1);
  len = sizeof (str_critical);
  ret = asn1_read_value (resp->basicresp, name, str_critical, &len);
  if (ret == ASN1_ELEMENT_NOT_FOUND)
    return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
  else if (ret != ASN1_SUCCESS)
    {
      gnutls_assert ();
      return _gnutls_asn2err (ret);
    }

  if (critical)
    {
      if (str_critical[0] == 'T')
	*critical = 1;
      else
	*critical = 0;
    }

  if (oid)
    {
      snprintf (name, sizeof (name),
		"tbsResponseData.responseExtensions.?%u.extnID", indx + 1);
      ret = _gnutls_x509_read_value (resp->basicresp, name, oid, 0);
      if (ret != GNUTLS_E_SUCCESS)
	{
	  gnutls_assert ();
	  return ret;
	}
    }

  if (data)
    {
      snprintf (name, sizeof (name),
		"tbsResponseData.responseExtensions.?%u.extnValue", indx + 1);
      ret = _gnutls_x509_read_value (resp->basicresp, name, data, 0);
      if (ret != GNUTLS_E_SUCCESS)
	{
	  gnutls_assert ();
	  if (oid)
	    gnutls_free (oid->data);
	  return ret;
	}
    }

  return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_ocsp_resp_get_nonce:
 * @resp: should contain a #gnutls_ocsp_resp_t structure
 * @critical: whether nonce extension is marked critical
 * @nonce: will hold newly allocated buffer with nonce data
 *
 * This function will return the Basic OCSP Response nonce extension
 * data.
 *
 * The caller needs to deallocate memory by calling gnutls_free() on
 * @nonce->data.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error code is returned.
 **/
int
gnutls_ocsp_resp_get_nonce (gnutls_ocsp_resp_t resp,
			    unsigned int *critical,
			    gnutls_datum_t *nonce)
{
  int ret;
  size_t l = 0;
  gnutls_datum_t tmp;

  ret = get_extension (resp->basicresp, "tbsResponseData.responseExtensions",
		       GNUTLS_OCSP_NONCE, 0,
		       &tmp, critical);
  if (ret != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      return ret;
    }

  ret = _gnutls_x509_decode_octet_string (NULL, tmp.data, (size_t) tmp.size,
					  NULL, &l);
  if (ret != GNUTLS_E_SHORT_MEMORY_BUFFER)
    {
      gnutls_assert ();
      gnutls_free (tmp.data);
      return ret;
    }

  nonce->data = gnutls_malloc (l);
  if (nonce->data == NULL)
    {
      gnutls_assert ();
      gnutls_free (tmp.data);
      return GNUTLS_E_MEMORY_ERROR;
    }

  ret = _gnutls_x509_decode_octet_string (NULL, tmp.data, (size_t) tmp.size,
					  nonce->data, &l);
  gnutls_free (tmp.data);
  if (ret != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      return ret;
    }
  nonce->size = l;

  return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_ocsp_resp_get_signature_algorithm:
 * @resp: should contain a #gnutls_ocsp_resp_t structure
 *
 * This function will return a value of the #gnutls_sign_algorithm_t
 * enumeration that is the signature algorithm that has been used to
 * sign the OCSP response.
 *
 * Returns: a #gnutls_sign_algorithm_t value, or a negative error code
 *   on error.
 **/
int
gnutls_ocsp_resp_get_signature_algorithm (gnutls_ocsp_resp_t resp)
{
  int ret;
  gnutls_datum_t sa;

  ret = _gnutls_x509_read_value (resp->basicresp,
				 "signatureAlgorithm.algorithm", &sa, 0);
  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  ret = _gnutls_x509_oid2sign_algorithm ((char*)sa.data);

  _gnutls_free_datum (&sa);

  return ret;
}

/**
 * gnutls_ocsp_resp_get_signature:
 * @resp: should contain a #gnutls_ocsp_resp_t structure
 * @sig: newly allocated output buffer with signature data
 *
 * This function will extract the signature field of a OCSP response.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_ocsp_resp_get_signature (gnutls_ocsp_resp_t resp,
				gnutls_datum_t *sig)
{
  int ret;

  if (resp == NULL || sig == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  ret = _gnutls_x509_read_value (resp->basicresp, "signature", sig, 2);
  if (ret != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      return ret;
    }

  return GNUTLS_E_SUCCESS;
}

/**
 * gnutls_ocsp_resp_get_certs:
 * @resp: should contain a #gnutls_ocsp_resp_t structure
 * @certs: newly allocated array with #gnutls_x509_crt_t certificates
 * @ncerts: output variable with number of allocated certs.
 *
 * This function will extract the X.509 certificates found in the
 * Basic OCSP Response.  The @certs output variable will hold a newly
 * allocated zero-terminated array with X.509 certificates.
 *
 * Every certificate in the array needs to be de-allocated with
 * gnutls_x509_crt_deinit() and the array itself must be freed using
 * gnutls_free().
 *
 * Both the @certs and @ncerts variables may be NULL.  Then the
 * function will work as normal but will not return the NULL:d
 * information.  This can be used to get the number of certificates
 * only, or to just get the certificate array without its size.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_ocsp_resp_get_certs (gnutls_ocsp_resp_t resp,
			    gnutls_x509_crt_t ** certs,
			    size_t *ncerts)
{
  int ret;
  size_t ctr = 0, i;
  gnutls_x509_crt_t *tmpcerts = NULL, *tmpcerts2;
  gnutls_datum_t c = { NULL, 0 };

  if (resp == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  tmpcerts = gnutls_malloc (sizeof (*tmpcerts));
  if (tmpcerts == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  for (;;)
    {
      char name[ASN1_MAX_NAME_SIZE];

      snprintf (name, sizeof (name), "certs.?%lu", ctr + 1);
      ret = _gnutls_x509_der_encode (resp->basicresp, name, &c, 0);
      if (ret == GNUTLS_E_ASN1_ELEMENT_NOT_FOUND)
	break;
      if (ret != GNUTLS_E_SUCCESS)
	{
	  gnutls_assert ();
	  goto error;
	}

      tmpcerts2 = gnutls_realloc (tmpcerts, (ctr + 2) * sizeof (*tmpcerts));
      if (tmpcerts2 == NULL)
	{
	  gnutls_assert ();
	  ret = GNUTLS_E_MEMORY_ERROR;
	  goto error;
	}
      tmpcerts = tmpcerts2;

      ret = gnutls_x509_crt_init (&tmpcerts[ctr]);
      if (ret != GNUTLS_E_SUCCESS)
	{
	  gnutls_assert ();
	  goto error;
	}
      ctr++;

      ret = gnutls_x509_crt_import (tmpcerts[ctr - 1], &c,
				    GNUTLS_X509_FMT_DER);
      if (ret != GNUTLS_E_SUCCESS)
	{
	  gnutls_assert ();
	  goto error;
	}

      gnutls_free (c.data);
      c.data = NULL;
    }

  tmpcerts[ctr] = NULL;

  if (ncerts)
    *ncerts = ctr;
  if (certs)
    *certs = tmpcerts;
  else
    {
      /* clean up memory */
      ret = GNUTLS_E_SUCCESS;
      goto error;
    }

  return GNUTLS_E_SUCCESS;

 error:
  gnutls_free (c.data);
  for (i = 0; i < ctr; i++)
    gnutls_x509_crt_deinit (tmpcerts[i]);
  gnutls_free (tmpcerts);
  return ret;
}

/* Search the OCSP response for a certificate matching the responderId
   mentioned in the OCSP response. */
static gnutls_x509_crt_t
find_signercert (gnutls_ocsp_resp_t resp)
{
  int rc;
  gnutls_x509_crt_t * certs;
  size_t ncerts = 0, i;
  gnutls_datum_t riddn;
  gnutls_x509_crt_t signercert = NULL;

  rc = gnutls_ocsp_resp_get_responder (resp, &riddn);
  if (rc != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      return NULL;
    }

  rc = gnutls_ocsp_resp_get_certs (resp, &certs, &ncerts);
  if (rc != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      gnutls_free (riddn.data);
      return NULL;
    }

  for (i = 0; i < ncerts; i++)
    {
      char *crtdn;
      size_t crtdnsize = 0;
      int cmpok;

      rc = gnutls_x509_crt_get_dn (certs[i], NULL, &crtdnsize);
      if (rc != GNUTLS_E_SHORT_MEMORY_BUFFER)
	{
	  gnutls_assert ();
	  goto quit;
	}

      crtdn = gnutls_malloc (crtdnsize);
      if (crtdn == NULL)
	{
	  gnutls_assert ();
	  goto quit;
	}

      rc = gnutls_x509_crt_get_dn (certs[i], crtdn, &crtdnsize);
      if (rc != GNUTLS_E_SUCCESS)
	{
	  gnutls_assert ();
	  gnutls_free (crtdn);
	  goto quit;
	}

      cmpok = (crtdnsize == riddn.size)
	&& memcmp (riddn.data, crtdn, crtdnsize);

      gnutls_free (crtdn);

      if (cmpok == 0)
	{
	  signercert = certs[i];
	  goto quit;
	}
    }

  gnutls_assert ();
  signercert = NULL;

 quit:
  gnutls_free (riddn.data);
  for (i = 0; i < ncerts; i++)
    if (certs[i] != signercert)
      gnutls_x509_crt_deinit (certs[i]);
  gnutls_free (certs);
  return signercert;
}

static int
_ocsp_resp_verify_direct (gnutls_ocsp_resp_t resp,
		          gnutls_x509_crt_t signercert,
			  unsigned int *verify,
			  unsigned int flags)
{
  gnutls_datum_t sig = { NULL };
  gnutls_datum_t data = { NULL };
  gnutls_pubkey_t pubkey = NULL;
  int sigalg;
  int rc;

  if (resp == NULL || signercert == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  rc = gnutls_ocsp_resp_get_signature_algorithm (resp);
  if (rc < 0)
    {
      gnutls_assert ();
      goto done;
    }
  sigalg = rc;

  rc = export (resp->basicresp, "tbsResponseData", &data);
  if (rc != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      goto done;
    }

  rc = gnutls_pubkey_init (&pubkey);
  if (rc != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      goto done;
    }

  rc = gnutls_pubkey_import_x509 (pubkey, signercert, 0);
  if (rc != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      goto done;
    }

  rc = gnutls_ocsp_resp_get_signature (resp, &sig);
  if (rc != GNUTLS_E_SUCCESS)
    {
      gnutls_assert ();
      goto done;
    }

  rc = gnutls_pubkey_verify_data2 (pubkey, sigalg, 0, &data, &sig);
  if (rc == GNUTLS_E_PK_SIG_VERIFY_FAILED)
    {
      gnutls_assert ();
      *verify = GNUTLS_OCSP_VERIFY_SIGNATURE_FAILURE;
    }
  else if (rc < 0)
    {
      gnutls_assert ();
      goto done;
    }
  else
    *verify = 0;

  rc = GNUTLS_E_SUCCESS;

 done:
  gnutls_free (data.data);
  gnutls_free (sig.data);
  gnutls_pubkey_deinit (pubkey);

  return rc;
}

static inline unsigned int vstatus_to_ocsp_status(unsigned int status)
{
unsigned int ostatus;

  if (status & GNUTLS_CERT_INSECURE_ALGORITHM)
    ostatus = GNUTLS_OCSP_VERIFY_INSECURE_ALGORITHM;
  else if (status & GNUTLS_CERT_NOT_ACTIVATED)
    ostatus = GNUTLS_OCSP_VERIFY_CERT_NOT_ACTIVATED;
  else if (status & GNUTLS_CERT_EXPIRED)
    ostatus = GNUTLS_OCSP_VERIFY_CERT_EXPIRED;
  else
    ostatus = GNUTLS_OCSP_VERIFY_UNTRUSTED_SIGNER;

  return ostatus;
}

static int check_ocsp_purpose(gnutls_x509_crt_t signercert)
{
char oidtmp[sizeof (GNUTLS_KP_OCSP_SIGNING)];
size_t oidsize;
int indx, rc;

      for (indx = 0; ; indx++)
	{
	  oidsize = sizeof (oidtmp);
	  rc = gnutls_x509_crt_get_key_purpose_oid (signercert, indx,
						    oidtmp, &oidsize,
						    NULL);
	  if (rc == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
	    {
	      gnutls_assert();
	      return rc;
	    }
	  else if (rc == GNUTLS_E_SHORT_MEMORY_BUFFER)
	    {
	      gnutls_assert ();
	      continue;
	    }
	  else if (rc != GNUTLS_E_SUCCESS)
	    {
	      return gnutls_assert_val(rc);
	    }

	  if (memcmp (oidtmp, GNUTLS_KP_OCSP_SIGNING, oidsize) != 0)
	    {
	      gnutls_assert ();
	      continue;
	    }
	  break;
	}
  
  return 0;
}

/**
 * gnutls_ocsp_resp_verify_direct:
 * @resp: should contain a #gnutls_ocsp_resp_t structure
 * @issuer: certificate believed to have signed the response
 * @verify: output variable with verification status, an #gnutls_ocsp_cert_status_t
 * @flags: verification flags, 0 for now.
 *
 * Verify signature of the Basic OCSP Response against the public key
 * in the @issuer certificate.
 *
 * The output @verify variable will hold verification status codes
 * (e.g., %GNUTLS_OCSP_VERIFY_SIGNER_NOT_FOUND,
 * %GNUTLS_OCSP_VERIFY_INSECURE_ALGORITHM) which are only valid if the
 * function returned %GNUTLS_E_SUCCESS.
 *
 * Note that the function returns %GNUTLS_E_SUCCESS even when
 * verification failed.  The caller must always inspect the @verify
 * variable to find out the verification status.
 *
 * The @flags variable should be 0 for now.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_ocsp_resp_verify_direct (gnutls_ocsp_resp_t resp,
				gnutls_x509_crt_t issuer,
				unsigned int *verify,
				unsigned int flags)
{
  gnutls_x509_crt_t signercert;
  int rc;

  if (resp == NULL || issuer == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  signercert = find_signercert (resp);
  if (!signercert)
    {
      signercert = issuer;
    }
  else /* response contains a signer. Verify him */
    {
      unsigned int vtmp;

      rc = gnutls_x509_crt_verify (signercert, &issuer, 1, 0, &vtmp);
      if (rc != GNUTLS_E_SUCCESS)
	{
	  gnutls_assert ();
	  goto done;
	}

      if (vtmp != 0)
        {
          *verify = vstatus_to_ocsp_status(vtmp);
	  gnutls_assert ();
	  rc = GNUTLS_E_SUCCESS;
	  goto done;
	}

      rc = check_ocsp_purpose(signercert);
      if (rc < 0)
        {
          gnutls_assert ();
          *verify = GNUTLS_OCSP_VERIFY_SIGNER_KEYUSAGE_ERROR;
          rc = GNUTLS_E_SUCCESS;
          goto done;
        }
    }

  rc = _ocsp_resp_verify_direct(resp, signercert, verify, flags);
  
 done:
  if (signercert != issuer)
    gnutls_x509_crt_deinit(signercert);

  return rc;
}

/**
 * gnutls_ocsp_resp_verify:
 * @resp: should contain a #gnutls_ocsp_resp_t structure
 * @trustlist: trust anchors as a #gnutls_x509_trust_list_t structure
 * @verify: output variable with verification status, an #gnutls_ocsp_cert_status_t
 * @flags: verification flags, 0 for now.
 *
 * Verify signature of the Basic OCSP Response against the public key
 * in the certificate of a trusted signer.  The @trustlist should be
 * populated with trust anchors.  The function will extract the signer
 * certificate from the Basic OCSP Response and will verify it against
 * the @trustlist.  A trusted signer is a certificate that is either
 * in @trustlist, or it is signed directly by a certificate in
 * @trustlist and has the id-ad-ocspSigning Extended Key Usage bit
 * set.
 *
 * The output @verify variable will hold verification status codes
 * (e.g., %GNUTLS_OCSP_VERIFY_SIGNER_NOT_FOUND,
 * %GNUTLS_OCSP_VERIFY_INSECURE_ALGORITHM) which are only valid if the
 * function returned %GNUTLS_E_SUCCESS.
 *
 * Note that the function returns %GNUTLS_E_SUCCESS even when
 * verification failed.  The caller must always inspect the @verify
 * variable to find out the verification status.
 *
 * The @flags variable should be 0 for now.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_ocsp_resp_verify (gnutls_ocsp_resp_t resp,
			 gnutls_x509_trust_list_t trustlist,
			 unsigned *verify,
			 int flags)
{
  gnutls_x509_crt_t signercert = NULL;
  int rc;

  /* Algorithm:
     1. Find signer cert.
        1a. Search in OCSP response Certificate field for responderID.
        1b. Verify that signer cert is trusted.
        2a. It is in trustlist?
        2b. It has OCSP key usage and directly signed by a CA in trustlist?
     3. Verify signature of Basic Response using public key from signer cert.
  */

  signercert = find_signercert (resp);
  if (!signercert)
    {
      /* XXX Search in trustlist for certificate matching
	 responderId as well? */
      gnutls_assert ();
      *verify = GNUTLS_OCSP_VERIFY_SIGNER_NOT_FOUND;
      rc = GNUTLS_E_SUCCESS;
      goto done;
    }

  /* Either the signer is directly trusted (i.e., in trustlist) or it
     is directly signed by something in trustlist and has proper OCSP
     extkeyusage. */
  rc = _gnutls_trustlist_inlist (trustlist, signercert);
  if (rc < 0)
    {
      gnutls_assert ();
      goto done;
    }
  if (rc == 1)
    {
      /* not in trustlist, need to verify signature and bits */
      gnutls_x509_crt_t issuer;
      unsigned vtmp;

      gnutls_assert ();

      rc = gnutls_x509_trust_list_get_issuer (trustlist, signercert,
					      &issuer, 0);
      if (rc != GNUTLS_E_SUCCESS)
	{
	  gnutls_assert ();
	  *verify = GNUTLS_OCSP_VERIFY_UNTRUSTED_SIGNER;
	  rc = GNUTLS_E_SUCCESS;
	  goto done;
	}

      rc = gnutls_x509_crt_verify (signercert, &issuer, 1, 0, &vtmp);
      if (rc != GNUTLS_E_SUCCESS)
	{
	  gnutls_assert ();
	  goto done;
	}

      if (vtmp != 0)
        {
          *verify = vstatus_to_ocsp_status(vtmp);
	  gnutls_assert ();
	  rc = GNUTLS_E_SUCCESS;
	  goto done;
	}

      rc = check_ocsp_purpose(signercert);
      if (rc < 0)
        {
          gnutls_assert ();
          *verify = GNUTLS_OCSP_VERIFY_SIGNER_KEYUSAGE_ERROR;
          rc = GNUTLS_E_SUCCESS;
          goto done;
        }
    }

  rc = _ocsp_resp_verify_direct (resp, signercert, verify, flags);

 done:
  gnutls_x509_crt_deinit (signercert);

  return rc;
}
