# vim: set et sts=4 sw=4 encoding=utf8:
#
# Copyright (c) 2008-2009, Giel van Schijndel
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the Warzone 2100 Project nor the names of its
#       contributors may be used to endorse or promote products derived from
#       this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

from trac.core import *
from trac.web.api import Request

from Cookie import Morsel, _semispacejoin, _getdate

__all__ = ['TracHttpOnlyAuthCookie']

# Python 2.6 already has support for the httponly attribute in Morsel
import sys
if sys.version_info < (2, 6):
	# Add the 'httponly' attribute
	Morsel._reserved['httponly'] = 'httponly'

	# Replacement for Morsel.OutputString that properly handles 'httponly' attributes
	def tmp_func(self, attrs=None):
	    # Build up our result
	    #
	    result = []
	    RA = result.append

	    # First, the key=value pair
	    RA("%s=%s" % (self.key, self.coded_value))

	    # Now add any defined attributes
	    if attrs is None:
		attrs = self._reserved
	    items = self.items()
	    items.sort()
	    for K,V in items:
		if V == "": continue
		if K not in attrs: continue
		if K == "expires" and type(V) == type(1):
		    RA("%s=%s" % (self._reserved[K], _getdate(V)))
		elif K == "max-age" and type(V) == type(1):
		    RA("%s=%d" % (self._reserved[K], V))
		elif K == "secure":
		    RA(str(self._reserved[K]))
		elif K == "httponly":
		    RA(str(self._reserved[K]))
		else:
		    RA("%s=%s" % (self._reserved[K], V))

	    # Return the result
	    return _semispacejoin(result)
	# end OutputString

	Morsel.OutputString = tmp_func
del sys

class TracHttpOnlyAuthCookie(Component):
    """Adds the 'HttpOnly' cookie attribute to the trac_auth cookie.

    This should provide for some additional level of safety by preventing
    access to this cookie from JavaScript code."""
    pass

_old_end_headers = Request.end_headers

# Hack up Request.end_headers such that it adds a 'httponly' cookie
# attribute to the 'trac_auth' cookie.
def end_headers(self):
    for cookie in ['trac_auth']:
        if cookie in self.outcookie:
            self.outcookie[cookie]['httponly'] = True
    _old_end_headers(self)

Request.end_headers = end_headers
