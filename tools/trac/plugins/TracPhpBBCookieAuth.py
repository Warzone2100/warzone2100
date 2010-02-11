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

from __future__ import with_statement
from contextlib import contextmanager
import re, md5
from trac.config import Option, BoolOption
from trac.core import *
from trac.web.api import IAuthenticator, IRequestFilter
from phpbbauth.main import PhpDatabaseManager

__all__ = ['TracPhpBBCookieAuth']

@contextmanager
def openPhpBBDatabase(env):
    cnx = PhpDatabaseManager(env).get_connection()
    try:
        yield cnx
    finally:
        cnx.close()

class TracPhpBBCookieAuth(Component):
    """Uses PhpBB cookies for authentication

    This enables a Single Sign On system between a PHPBB3 Forum and Trac."""

    ignore_case = BoolOption('trac', 'ignore_auth_case', 'false',
        """Whether login names should be converted to lower case
        (''since 0.9'').""")

    cookie_domain_option = Option('account-manager', 'phpbb_cookie_domain', None, 
                            """Domain that's used for phpBB's cookies""")
    cookie_domain = property(fget=lambda self: str(self.cookie_domain_option))

    cookie_name = Option('account-manager', 'phpbb_cookie_name', None, 
                         """Prefix name that's used for phpBB's cookies""")

    sid_cookie = property(fget=lambda self: '%s_sid' % str(self.cookie_name))
    u_cookie = property(fget=lambda self: '%s_u' % str(self.cookie_name))
    k_cookie = property(fget=lambda self: '%s_k' % str(self.cookie_name))

    implements(IAuthenticator, IRequestFilter)

    # IAuthenticator methods

    def authenticate(self, req):
        authname = None
        # Use existing phpBB3 sessions if there is one
        if self.sid_cookie in req.incookie:
            with openPhpBBDatabase(self.env) as cnx:
                cur = cnx.cursor()
                cur.execute('SELECT u.username'
                            '  FROM phpbb3_sessions s '
                            '    INNER JOIN phpbb3_users u '
                            '      ON s.session_user_id = u.user_id'
                            ' WHERE user_type <> 2 '
                            '   AND s.session_id = %s', (req.incookie[self.sid_cookie].value,))
                result = cur.fetchone()
                authname = result and result[0] or None
        if self.k_cookie in req.incookie and self.u_cookie in req.incookie \
         and not authname:
            key_id = md5.new(req.incookie[self.k_cookie].value).hexdigest()

            with openPhpBBDatabase(self.env) as cnx:
                cur = cnx.cursor()
                cur.execute('SELECT u.username'
                            '  FROM phpbb3_sessions_keys k '
                            '    INNER JOIN phpbb3_users u '
                            '      ON k.user_id = u.user_id'
                            ' WHERE u.user_id = %s'
                            '   AND u.user_type <> 2 '
                            '   AND k.key_id = %s', (req.incookie[self.u_cookie].value, key_id))
                result = cur.fetchone()
                authname = result and result[0] or None

        if not authname:
            return None

        if self.ignore_case:
            authname = authname.lower()

        return authname

    # IRequestFilter methods

    def pre_process_request(self, req, handler):
        # Destroy existing phpBB3 sessions when logging out
        if re.match('/logout/?$', req.path_info) \
         and self.sid_cookie in req.incookie and self.u_cookie in req.incookie:
            with openPhpBBDatabase(self.env) as cnx:
                cur = cnx.cursor()

                # Clean up the session itself
                cur.execute('DELETE FROM phpbb3_sessions '
                            ' WHERE session_id = %s AND session_user_id = %s',
                            (req.incookie[self.sid_cookie].value,
                                req.incookie[self.u_cookie].value))

                # Remove the session key to prevent it from being recreated
                if self.k_cookie in req.incookie:
                    cur.execute('DELETE FROM phpbb3_sessions_keys '
                                ' WHERE user_id = %s AND key_id = %s',
                                (req.incookie[self.u_cookie].value,
                                    req.incookie[self.k_cookie].value))

                # Destroy all phpBB3 session-related cookies
                req.outcookie[self.k_cookie] = ''
                req.outcookie[self.k_cookie]['domain'] = self.cookie_domain
                req.outcookie[self.k_cookie]['expires'] = -10000
                req.outcookie[self.sid_cookie] = ''
                req.outcookie[self.sid_cookie]['domain'] = self.cookie_domain
                req.outcookie[self.sid_cookie]['expires'] = -10000
                req.outcookie[self.u_cookie] = ''
                req.outcookie[self.u_cookie]['domain'] = self.cookie_domain
                req.outcookie[self.u_cookie]['expires'] = -10000

        return handler

    # for Genshi templates
    def post_process_request(self, req, template, data, content_type):
        return template, data, content_type
