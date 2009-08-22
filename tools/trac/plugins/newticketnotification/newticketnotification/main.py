# vim: set et sts=4 sw=4 encoding=utf8:
# -*- coding: utf-8 -*-

from model import *

from trac.config import Option
from trac.core import *
from trac.ticket.api import ITicketChangeListener
from trac.ticket.notification import TicketNotifyEmail
from trac.util.text import exception_to_unicode

class TicketCCNotifyEmail(TicketNotifyEmail):
    cc_on_newticket = Option('notification', 'cc_on_newticket', '',
        """Email address(es) to always send notifications to when new tickets
           are created, addresses can be seen by all recipients (Cc:).""")

    def send(self, torcpts, ccrcpts, mime_headers={}):
        public_cc = self.config.getbool('notification', 'use_public_cc')
        if not public_cc and not 'Cc' in mime_headers and self.ccrecipients:
            mime_headers['Cc'] = ', '.join(self.ccrecipients)
        TicketNotifyEmail.send(self, torcpts, ccrcpts)

    @property
    def torecipients(self):
        return []

    @property
    def ccrecipients(self):
        return get_cc_recipients(self.config)

    def get_recipients(self, tktid):
        return (self.torecipients, self.ccrecipients)

class NewTicketNotification(Component):
    """Extends Trac to notify a configured set of e-mail addresses upon ticket
    creation.
    """

    implements(ITicketChangeListener)

    # ITicketChangeListener methods
    def ticket_created(self, ticket):
        """Called when a ticket is created."""
        try:
            tn = TicketCCNotifyEmail(self.env)
            tn.notify(ticket, newticket=True)
        except Exception, e:
            self.log.exception("Failure sending notification on creation of "
                    "ticket #%s: %s", ticket.id, exception_to_unicode(e))

    def ticket_changed(self, ticket, comment, author, old_values):
        pass

    def ticket_deleted(self, ticket):
        pass
