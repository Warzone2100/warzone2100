# -*- coding: utf-8 -*-
#
# Copyright (C) 2006 Edgewall Software
# All rights reserved.
#
# This software is licensed as described in the file COPYING, which
# you should have received as part of this distribution. The terms
# are also available at http://trac.edgewall.com/license.html.
#
# This software consists of voluntary contributions made by many
# individuals. For the exact contribution history, see the revision
# history and logs, available at http://projects.edgewall.com/trac/.

from email.Utils import parseaddr
import re
import rfc822

from trac.config import IntOption
from trac.core import *
from tracspamfilter.api import IFilterStrategy

class ValidEmailFilterStrategy(Component):
    """This strategy grants positive karma points to anonymous users with a valid RFC822 e-mail address."""

    implements(IFilterStrategy)

    karma_points = IntOption('ticketvalidemail', 'validemail_karma', '10',
        """By how many points a valid RFC822 e-mail address improves the overall karma of
	the submission for anonymous users. Invalid e-mail will lower karma by that value.""")

    reject_emails_re = re.compile(r'^\w+@example\.(org|net|com)$')

    # IFilterStrategy implementation

    def is_external(self):
        return False

    def test(self, req, author, content, ip):
        points = -abs(self.karma_points)

        if req.authname and req.authname != 'anonymous':
           self.log.debug("Authenticated user, skipping...")
           return

        # Split up author into name and email, if possible
        author = author.encode('utf-8')
        author_name, author_email = parseaddr(author)
        self.log.debug("Author name is [%s] and e-mail is [%s]", author_name, author_email)

        if not author_email:
           return points, 'No e-mail found'
        elif self.reject_emails_re.match(author_email.lower()):
           return points, 'Example e-mail detected'
        elif rfc822.valid(author_email):
           points = abs(self.karma_points)
           return points, 'Valid e-mail found'
        else:
           return points, 'No valid RFC822 e-mail address found in reporter field'

    def train(self, req, author, content, ip, spam=True):
        pass
