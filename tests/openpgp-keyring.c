/*
 * Copyright (C) 2007 Free Software Foundation, Inc.
 * Author: Ludovic Court�s, Timo Schulz
 *
 * This file is part of GNUTLS.
 *
 * GNUTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GNUTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNUTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#include <gnutls/gnutls.h>
#include <gnutls/extra.h>
#include <gnutls/openpgp.h>

#include "utils.h"

/* A hex-encoded raw OpenPGP keyring.  This is a copy of (`sha1sum' output):
   5fdce61bff528070dfabdd237d91be618c353b4e  src/openpgp/cli_ring.gpg  */
static unsigned char raw_keyring[] = {
  0x99, 0x01, 0xA2, 0x04, 0x3C, 0x67, 0x95, 0x8D, 0x11, 0x04, 0x00, 0x80,
  0xB1, 0x65, 0x21, 0x8B, 0xF8, 0x28, 0x06, 0xFA, 0x6F, 0x4C, 0x18, 0x0B,
  0xF1, 0xF1, 0x4F, 0xC0, 0x10, 0x2E, 0x0F, 0x4E, 0x15, 0x60, 0x51, 0x2D,
  0x0B, 0xBF, 0xB8, 0xA4, 0x1A, 0x7A, 0x90, 0x5B, 0x07, 0x8D, 0x44, 0x7B,
  0x4D, 0x35, 0x24, 0x06, 0xC3, 0xA4, 0xD8, 0xFB, 0xCC, 0x1E, 0xB0, 0xDD,
  0xBF, 0x4F, 0x82, 0xE3, 0x1D, 0x82, 0x1F, 0xC6, 0x06, 0x3F, 0x57, 0xBE,
  0x3B, 0x47, 0xF6, 0xC8, 0xB5, 0xA4, 0xF1, 0x4B, 0xBE, 0x92, 0x41, 0x75,
  0xDB, 0x28, 0xAA, 0x6D, 0xBB, 0xC3, 0x12, 0x20, 0x9D, 0x78, 0x94, 0xFA,
  0x73, 0x7B, 0xC8, 0xB2, 0xD6, 0x3C, 0xBC, 0x9F, 0x49, 0xB2, 0x8E, 0x60,
  0xFC, 0xB0, 0x7C, 0x5E, 0x08, 0x2A, 0xF3, 0xC4, 0x7B, 0x8D, 0x71, 0x52,
  0xDE, 0x11, 0xFE, 0x58, 0x2E, 0x6F, 0xFF, 0xA3, 0xFA, 0x48, 0x04, 0x5F,
  0xCD, 0x79, 0x78, 0xE7, 0xB7, 0x15, 0x7B, 0x00, 0xA0, 0xBF, 0x14, 0x9F,
  0x1A, 0xC9, 0xBD, 0x98, 0x5A, 0x2C, 0xA4, 0x9D, 0x01, 0xDD, 0x11, 0xB2,
  0x83, 0x93, 0x01, 0xD1, 0xDF, 0x03, 0xFD, 0x14, 0x10, 0xAF, 0x22, 0x42,
  0x19, 0xD4, 0x76, 0x9C, 0xB7, 0xB8, 0x55, 0xF7, 0x2D, 0x3C, 0xBD, 0x90,
  0x04, 0x3F, 0xF5, 0x5E, 0x1B, 0x6E, 0x6E, 0xA1, 0x1B, 0x7A, 0xD6, 0x95,
  0x3F, 0x1B, 0x2C, 0xAA, 0xB2, 0x5D, 0x03, 0xE7, 0xA9, 0x94, 0x14, 0x53,
  0xED, 0x41, 0xE8, 0x91, 0x20, 0x5A, 0x84, 0xCF, 0x20, 0x99, 0x29, 0x8D,
  0xB9, 0x2A, 0xCB, 0x0E, 0xE8, 0xCF, 0x7C, 0x4B, 0x5A, 0x32, 0x0E, 0x98,
  0x22, 0x40, 0x7E, 0x2A, 0xAD, 0x15, 0x78, 0x92, 0xC4, 0xD1, 0xC5, 0xD3,
  0x64, 0x81, 0xF6, 0xF4, 0xA2, 0x65, 0x23, 0xFA, 0xA4, 0xD7, 0x11, 0xB8,
  0x2B, 0xB0, 0xFA, 0x07, 0x47, 0x0A, 0x68, 0x70, 0xBF, 0x2F, 0x80, 0x48,
  0xA0, 0xA7, 0x10, 0x2C, 0x9C, 0xDF, 0x4C, 0x83, 0xF0, 0xDD, 0xFA, 0xD2,
  0xE2, 0x35, 0x5E, 0x35, 0xA4, 0x19, 0x34, 0x74, 0x95, 0xA9, 0x9F, 0x3F,
  0x56, 0x63, 0x8C, 0x03, 0xFF, 0x6B, 0x90, 0xDB, 0x5C, 0x71, 0x0E, 0x11,
  0x55, 0xDF, 0x56, 0x4C, 0x5A, 0x07, 0x2A, 0xF4, 0xF8, 0xBD, 0xF8, 0x88,
  0x48, 0x43, 0x88, 0xCC, 0xA1, 0xA6, 0x70, 0x16, 0x3D, 0x1F, 0x29, 0xAA,
  0xEC, 0xC0, 0x9C, 0x8B, 0x79, 0x8D, 0x7B, 0x80, 0x83, 0x22, 0x69, 0x2F,
  0x66, 0x09, 0xE3, 0x0E, 0x52, 0x40, 0x33, 0xDD, 0x42, 0x5F, 0x53, 0x83,
  0xB6, 0x13, 0xCB, 0x06, 0xAB, 0xF2, 0x86, 0x73, 0x21, 0x87, 0x10, 0xE7,
  0x68, 0x39, 0x78, 0x36, 0x1E, 0x36, 0xB8, 0xF3, 0x12, 0xAF, 0xD2, 0x44,
  0x5B, 0x62, 0x30, 0xA0, 0x86, 0xC5, 0x9D, 0xED, 0x74, 0x8A, 0x11, 0x93,
  0x3B, 0x89, 0x41, 0x4B, 0x50, 0xB6, 0xF1, 0x47, 0xD2, 0x18, 0x43, 0x26,
  0xFF, 0xC2, 0x41, 0x32, 0xDC, 0x40, 0x8D, 0xB6, 0x32, 0xDC, 0x16, 0x33,
  0x52, 0xD0, 0x8C, 0x03, 0xE6, 0xC6, 0x04, 0x6E, 0x95, 0xA1, 0xEE, 0x62,
  0xE4, 0xB4, 0x25, 0x44, 0x72, 0x2E, 0x20, 0x57, 0x68, 0x6F, 0x20, 0x28,
  0x4E, 0x6F, 0x20, 0x63, 0x6F, 0x6D, 0x6D, 0x65, 0x6E, 0x74, 0x73, 0x29,
  0x20, 0x3C, 0x77, 0x68, 0x6F, 0x40, 0x77, 0x68, 0x6F, 0x69, 0x73, 0x2E,
  0x6F, 0x72, 0x67, 0x3E, 0x88, 0x5D, 0x04, 0x13, 0x11, 0x02, 0x00, 0x1D,
  0x05, 0x02, 0x3C, 0x67, 0x95, 0x8D, 0x05, 0x09, 0x03, 0xC2, 0x67, 0x00,
  0x05, 0x0B, 0x07, 0x0A, 0x03, 0x04, 0x03, 0x15, 0x03, 0x02, 0x03, 0x16,
  0x02, 0x01, 0x02, 0x17, 0x80, 0x00, 0x0A, 0x09, 0x10, 0x35, 0x14, 0x5C,
  0xEA, 0xA7, 0xD9, 0x3C, 0x3F, 0x96, 0x58, 0x00, 0x9F, 0x78, 0x99, 0xCB,
  0xC9, 0xF6, 0xE9, 0x4C, 0x30, 0x7B, 0x98, 0x38, 0x77, 0x68, 0x04, 0xDB,
  0xFB, 0x43, 0xD7, 0xCF, 0x6F, 0x00, 0xA0, 0xA4, 0x5D, 0x02, 0x90, 0x55,
  0x33, 0xA0, 0x6D, 0xCB, 0xEB, 0xD6, 0xC9, 0x71, 0xFA, 0x1D, 0xF1, 0x7A,
  0x65, 0x38, 0xFE, 0x99, 0x01, 0xA2, 0x04, 0x3C, 0x4A, 0xC5, 0x6C, 0x11,
  0x04, 0x00, 0xE7, 0x2E, 0x76, 0xB6, 0x2E, 0xEF, 0xA9, 0xA3, 0xBD, 0x59,
  0x40, 0x93, 0x29, 0x24, 0x18, 0x05, 0x0C, 0x02, 0xD7, 0x02, 0x9D, 0x6C,
  0xA2, 0x06, 0x6E, 0xFC, 0x34, 0xC8, 0x60, 0x38, 0x62, 0x7C, 0x64, 0x3E,
  0xB1, 0xA6, 0x52, 0xA7, 0xAF, 0x1D, 0x37, 0xCF, 0x46, 0xFC, 0x50, 0x5A,
  0xC1, 0xE0, 0xC6, 0x99, 0xB3, 0x78, 0x95, 0xB4, 0xBC, 0xB3, 0xE5, 0x35,
  0x41, 0xFF, 0xDA, 0x47, 0x66, 0xD6, 0x16, 0x8C, 0x2B, 0x8A, 0xAF, 0xD6,
  0xAB, 0x22, 0x46, 0x6D, 0x06, 0xD1, 0x80, 0x34, 0xD5, 0xDA, 0xC6, 0x98,
  0xE6, 0x99, 0x3B, 0xA5, 0xB3, 0x50, 0xFF, 0x82, 0x2E, 0x1C, 0xD8, 0x70,
  0x2A, 0x75, 0x11, 0x4E, 0x8B, 0x73, 0xA6, 0xB0, 0x9C, 0xB3, 0xB9, 0x3C,
  0xE4, 0x4D, 0xBB, 0x51, 0x6C, 0x9B, 0xB5, 0xF9, 0x5B, 0xB6, 0x66, 0x18,
  0x86, 0x02, 0xA0, 0xA1, 0x44, 0x72, 0x36, 0xC0, 0x65, 0x8F, 0x00, 0xA0,
  0x8F, 0x5B, 0x5E, 0x78, 0xD8, 0x5F, 0x79, 0x2C, 0xC2, 0x07, 0x2F, 0x94,
  0x74, 0x64, 0x57, 0x26, 0xFB, 0x4D, 0x93, 0x73, 0x03, 0xFE, 0x35, 0x78,
  0xD6, 0x89, 0xD6, 0x60, 0x6E, 0x91, 0x18, 0xE9, 0xF9, 0xA7, 0x04, 0x2B,
  0x96, 0x3C, 0xF2, 0x3F, 0x3D, 0x8F, 0x13, 0x77, 0xA2, 0x73, 0xC0, 0xF0,
  0x97, 0x4D, 0xBF, 0x44, 0xB3, 0xCA, 0xBC, 0xBE, 0x14, 0xDD, 0x64, 0x41,
  0x25, 0x55, 0x86, 0x3E, 0x39, 0xA9, 0xC6, 0x27, 0x66, 0x2D, 0x77, 0xAC,
  0x36, 0x66, 0x2A, 0xE4, 0x49, 0x79, 0x2C, 0x32, 0x62, 0xD3, 0xF1, 0x2E,
  0x98, 0x32, 0xA7, 0x56, 0x53, 0x09, 0xD6, 0x7B, 0xA0, 0xAE, 0x4D, 0xF2,
  0x5F, 0x5E, 0xDA, 0x09, 0x37, 0x05, 0x6A, 0xD5, 0xBE, 0x89, 0xF4, 0x06,
  0x9E, 0xBD, 0x7E, 0xC7, 0x6C, 0xE4, 0x32, 0x44, 0x1D, 0xF5, 0xD5, 0x2F,
  0xFF, 0xD0, 0x6D, 0x39, 0xE5, 0xF6, 0x1E, 0x36, 0x94, 0x7B, 0x69, 0x8A,
  0x77, 0xCB, 0x62, 0xAB, 0x81, 0xE4, 0xA4, 0x12, 0x2B, 0xF9, 0x05, 0x06,
  0x71, 0xD9, 0x94, 0x6C, 0x86, 0x5E, 0x04, 0x00, 0xD0, 0x61, 0x43, 0x7A,
  0x96, 0x4D, 0xDE, 0x31, 0x88, 0x18, 0xC2, 0xB2, 0x4D, 0xE0, 0x08, 0xE6,
  0x00, 0x96, 0xB6, 0x0D, 0xB8, 0xA6, 0x84, 0xB8, 0x5A, 0x83, 0x8D, 0x11,
  0x9F, 0xC9, 0x30, 0x31, 0x18, 0x89, 0xAD, 0x57, 0xA3, 0xB9, 0x27, 0xF4,
  0x48, 0xF8, 0x4E, 0xB2, 0x53, 0xC6, 0x23, 0xED, 0xA7, 0x3B, 0x42, 0xFF,
  0x78, 0xBC, 0xE6, 0x3A, 0x6A, 0x53, 0x1D, 0x75, 0xA6, 0x4C, 0xE8, 0x54,
  0x05, 0x13, 0x80, 0x8E, 0x9F, 0x5B, 0x10, 0xCE, 0x07, 0x5D, 0x34, 0x17,
  0xB8, 0x01, 0x16, 0x49, 0x18, 0xB1, 0x31, 0xD3, 0x54, 0x4C, 0x87, 0x65,
  0xA8, 0xEC, 0xB9, 0x97, 0x1F, 0x61, 0xA0, 0x9F, 0xC7, 0x3D, 0x50, 0x98,
  0x06, 0x10, 0x6B, 0x59, 0x77, 0xD2, 0x11, 0xCB, 0x0E, 0x1D, 0x04, 0xD0,
  0xED, 0x96, 0xBC, 0xE8, 0x9B, 0xAE, 0x8F, 0x73, 0xD8, 0x00, 0xB0, 0x52,
  0x13, 0x9C, 0xBF, 0x8D, 0xB4, 0x49, 0x4F, 0x70, 0x65, 0x6E, 0x43, 0x44,
  0x4B, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x6B, 0x65, 0x79, 0x20, 0x28,
  0x4F, 0x6E, 0x6C, 0x79, 0x20, 0x69, 0x6E, 0x74, 0x65, 0x6E, 0x64, 0x65,
  0x64, 0x20, 0x66, 0x6F, 0x72, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x70,
  0x75, 0x72, 0x70, 0x6F, 0x73, 0x65, 0x73, 0x21, 0x29, 0x20, 0x3C, 0x6F,
  0x70, 0x65, 0x6E, 0x63, 0x64, 0x6B, 0x40, 0x66, 0x6F, 0x6F, 0x2D, 0x62,
  0x61, 0x72, 0x2E, 0x6F, 0x72, 0x67, 0x3E, 0x88, 0x62, 0x04, 0x13, 0x11,
  0x02, 0x00, 0x1A, 0x05, 0x02, 0x3C, 0x4A, 0xC5, 0x6C, 0x05, 0x0B, 0x07,
  0x0A, 0x03, 0x04, 0x03, 0x15, 0x03, 0x02, 0x03, 0x16, 0x02, 0x01, 0x02,
  0x1E, 0x01, 0x02, 0x17, 0x80, 0x00, 0x12, 0x09, 0x10, 0xBD, 0x57, 0x2C,
  0xDC, 0xCC, 0xC0, 0x7C, 0x35, 0x07, 0x65, 0x47, 0x50, 0x47, 0x00, 0x01,
  0x01, 0x81, 0xC1, 0x00, 0x9C, 0x0E, 0x12, 0x8D, 0x8E, 0xD4, 0x44, 0x7C,
  0x6D, 0xCB, 0xCE, 0x61, 0x50, 0xD9, 0xCD, 0x86, 0xE2, 0x0D, 0x84, 0x59,
  0xA5, 0x00, 0x9F, 0x66, 0x81, 0x66, 0x2C, 0x80, 0xC6, 0xAA, 0xCF, 0x1D,
  0x2D, 0x2B, 0xC2, 0x04, 0xF0, 0x82, 0xFE, 0x80, 0xD3, 0xDB, 0xA4, 0xB9,
  0x01, 0x0D, 0x04, 0x3C, 0x4A, 0xC5, 0x6F, 0x10, 0x04, 0x00, 0xE2, 0x01,
  0x56, 0x52, 0x60, 0x69, 0xD0, 0x67, 0xD2, 0x4F, 0x4D, 0x71, 0xE6, 0xD3,
  0x86, 0x58, 0xE0, 0x8B, 0xE3, 0xBF, 0x24, 0x6C, 0x1A, 0xDC, 0xE0, 0x8D,
  0xB6, 0x9C, 0xD8, 0xD4, 0x59, 0xC1, 0xED, 0x33, 0x57, 0x38, 0x41, 0x07,
  0x98, 0x75, 0x5A, 0xFD, 0xB7, 0x9F, 0x17, 0x97, 0xCF, 0x02, 0x2E, 0x70,
  0xC7, 0x96, 0x0F, 0x12, 0xCA, 0x68, 0x96, 0xD2, 0x7C, 0xFD, 0x24, 0xA1,
  0x1C, 0xD3, 0x16, 0xDD, 0xE1, 0xFB, 0xCC, 0x1E, 0xA6, 0x15, 0xC5, 0xC3,
  0x1F, 0xEC, 0x65, 0x6E, 0x46, 0x70, 0x78, 0xC8, 0x75, 0xFC, 0x50, 0x9B,
  0x1E, 0xCB, 0x99, 0xC8, 0xB5, 0x6C, 0x2D, 0x87, 0x5C, 0x50, 0xE2, 0x01,
  0x8B, 0x5B, 0x0F, 0xA3, 0x78, 0x60, 0x6E, 0xB6, 0x42, 0x5A, 0x25, 0x33,
  0x83, 0x0F, 0x55, 0xFD, 0x21, 0xD6, 0x49, 0x01, 0x56, 0x15, 0xD4, 0x9A,
  0x1D, 0x09, 0xE9, 0x51, 0x0F, 0x5F, 0x00, 0x03, 0x05, 0x04, 0x00, 0xD0,
  0xBD, 0xAD, 0xE4, 0x04, 0x32, 0x75, 0x86, 0x75, 0xC8, 0x7D, 0x07, 0x30,
  0xC3, 0x60, 0x98, 0x14, 0x67, 0xBA, 0xE1, 0xBE, 0xB6, 0xCC, 0x10, 0x5A,
  0x3C, 0x1F, 0x36, 0x6B, 0xFD, 0xBE, 0xA1, 0x2E, 0x37, 0x84, 0x56, 0x51,
  0x32, 0x38, 0xB8, 0xAD, 0x41, 0x4E, 0x52, 0xA2, 0xA9, 0x66, 0x1D, 0x1D,
  0xF1, 0xDB, 0x6B, 0xB5, 0xF3, 0x3F, 0x69, 0x06, 0x16, 0x61, 0x07, 0x55,
  0x6C, 0x81, 0x32, 0x24, 0x33, 0x0B, 0x30, 0x93, 0x2D, 0xB7, 0xC8, 0xCC,
  0x82, 0x25, 0x67, 0x2D, 0x7A, 0xE2, 0x4A, 0xF2, 0x46, 0x97, 0x50, 0xE5,
  0x39, 0xB6, 0x61, 0xEA, 0x64, 0x75, 0xD2, 0xE0, 0x3C, 0xD8, 0xD3, 0x83,
  0x8D, 0xC4, 0xA8, 0xAC, 0x4A, 0xFD, 0x21, 0x35, 0x36, 0xFE, 0x3E, 0x96,
  0xEC, 0x9D, 0x0A, 0xEA, 0x65, 0x16, 0x4B, 0x57, 0x6E, 0x01, 0xB3, 0x7A,
  0x8D, 0xCA, 0x89, 0xF2, 0xB2, 0x57, 0xD0, 0x88, 0x4E, 0x04, 0x18, 0x11,
  0x02, 0x00, 0x06, 0x05, 0x02, 0x3C, 0x4A, 0xC5, 0x6F, 0x00, 0x12, 0x09,
  0x10, 0xBD, 0x57, 0x2C, 0xDC, 0xCC, 0xC0, 0x7C, 0x35, 0x07, 0x65, 0x47,
  0x50, 0x47, 0x00, 0x01, 0x01, 0x75, 0x66, 0x00, 0x9F, 0x60, 0x1E, 0x1F,
  0x99, 0xE0, 0xB0, 0x7C, 0x77, 0xE6, 0x7F, 0x3E, 0xEC, 0xA1, 0xE1, 0x9F,
  0x94, 0x63, 0xD3, 0x73, 0x67, 0x00, 0x9F, 0x6A, 0xC6, 0x9E, 0xB4, 0x11,
  0x9A, 0x6F, 0xFB, 0xF4, 0x49, 0xE7, 0xD1, 0x54, 0xD8, 0x2E, 0x05, 0xD4,
  0x08, 0x61, 0xDB
};

/* The ID of a key known to be in the above keyring.  */
static const gnutls_openpgp_keyid_t id_in_keyring =
  /* "Dr. Who", first key in the keyring */
{ 0x35, 0x14, 0x5c, 0xea,
  0xa7, 0xd9, 0x3c, 0x3f
};

static const gnutls_openpgp_keyid_t id2_in_keyring =
  /* OpenCDK test key, second key in the keyring */
{ 0xbd, 0x57, 0x2c, 0xdc,
  0xcc, 0xc0, 0x7c, 0x35
};

static const gnutls_openpgp_keyid_t id_not_in_keyring =
  { 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};


static void
tls_log_func (int level, const char *str)
{
  fprintf (stderr, "%d| %s", level, str);
}

void
doit (void)
{
  gnutls_openpgp_keyring_t keyring;
  gnutls_datum_t data;
  int ret;

  ret = gnutls_global_init ();
  if (ret < 0)
    fail ("init %d\n", ret);

  gnutls_global_set_log_function (tls_log_func);
  gnutls_global_set_log_level (2);

  ret = gnutls_global_init_extra ();
  if (ret < 0)
    fail ("extra-init %d\n", ret);

  ret = gnutls_openpgp_keyring_init (&keyring);
  if (ret < 0)
    fail ("keyring-init %d\n", ret);

  data.data = raw_keyring;
  data.size = sizeof (raw_keyring) / sizeof (raw_keyring[0]);
  ret = gnutls_openpgp_keyring_import (keyring, &data,
				       GNUTLS_OPENPGP_FMT_RAW);
  if (ret < 0)
    fail ("keyring-import %d\n", ret);

  ret = gnutls_openpgp_keyring_check_id (keyring, id_not_in_keyring, 0);
  if (ret == 0)
    fail ("keyring-check-id (not-in-keyring) %d\n", ret);

  ret = gnutls_openpgp_keyring_check_id (keyring, id_in_keyring, 0);
  if (ret != 0)
    fail ("keyring-check-id first key %d\n", ret);

  ret = gnutls_openpgp_keyring_check_id (keyring, id2_in_keyring, 0);
  if (ret != 0)
    fail ("keyring-check-id second key %d\n", ret);

  success ("done\n");

  gnutls_openpgp_keyring_deinit (keyring);
  gnutls_global_deinit ();
}

/* Local Variables:
   coding: latin-1
   End:
 */
