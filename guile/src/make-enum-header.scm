;;; Help produce Guile wrappers for GnuTLS types.
;;;
;;; GNUTLS --- Guile bindings for GnuTLS.
;;; Copyright (C) 2007, 2008  Free Software Foundation, Inc.
;;;
;;; GNUTLS is free software; you can redistribute it and/or
;;; modify it under the terms of the GNU Lesser General Public
;;; License as published by the Free Software Foundation; either
;;; version 2.1 of the License, or (at your option) any later version.
;;;
;;; GNUTLS is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;;; Lesser General Public License for more details.
;;;
;;; You should have received a copy of the GNU Lesser General Public
;;; License along with GNUTLS; if not, write to the Free Software
;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

;;; Written by Ludovic Court�s <ludo@gnu.org>.


(use-modules (gnutls build enums))


;;;
;;; The program.
;;;

(define (main . args)
  (define extra? (not (null? args)))

  (let ((port (current-output-port))
        (enums (if (not extra?)
                   %gnutls-enums
                   %gnutls-extra-enums)))
    (format port "/* Automatically generated, do not edit.  */~%~%")
    (format port "#ifndef GUILE_GNUTLS_~aENUMS_H~%"
            (if extra? "EXTRA_" ""))
    (format port "#define GUILE_GNUTLS_~aENUMS_H~%"
            (if extra? "EXTRA_" ""))

    (format port "#ifdef HAVE_CONFIG_H~%")
    (format port "# include <config.h>~%")
    (format port "#endif~%~%")
    (format port "#include <gnutls/gnutls.h>~%")
    (format port "#include <gnutls/x509.h>~%")

    (if extra?
        (begin
          (format port "#include <gnutls/extra.h>~%")
          (format port "#include <gnutls/openpgp.h>~%")))

    (for-each (lambda (enum)
                (output-enum-declarations enum port)
                (output-enum->c-converter enum port)
                (output-c->enum-converter enum port))
              enums)
    (format port "#endif~%")))

(apply main (cdr (command-line)))

;;; Local Variables:
;;; mode: scheme
;;; coding: latin-1
;;; End:

;;; arch-tag: 07d834ca-e823-4663-9143-6d22704fbb5b
