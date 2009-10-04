# vim: set et sts=4 sw=4 encoding=utf8:
# -*- coding: utf-8 -*-

from trac.attachment import IAttachmentChangeListener
from trac.core import *
from trac.ticket import Ticket
from trac.ticket.notification import TicketNotifyEmail
from trac.util.text import exception_to_unicode
from trac.web.chrome import ITemplateProvider

class TicketAttachmentNotifyEmail(TicketNotifyEmail):
    template_name = 'attachment_email.txt'

class AttachmentTicketNotification(Component):
    """Extends Trac to notify ticket subscribers when a new attachment is
    created and attached to a ticket.
    """

    implements(ITemplateProvider, IAttachmentChangeListener)

    # ITemplateProvider methods
    def get_htdocs_dirs(self):
        """Return a list of directories with static resources (such as style
        sheets, images, etc.)

        Each item in the list must be a `(prefix, abspath)` tuple. The
        `prefix` part defines the path in the URL that requests to these
        resources are prefixed with.
        
        The `abspath` is the absolute path to the directory containing the
        resources on the local file system.
        """
        return []

    def get_templates_dirs(self):
        """Return a list of directories containing the provided template
        files.
        """
        from pkg_resources import resource_filename
        return [resource_filename(__name__, 'templates')]

    # IAttachmentChangeListener methods
    def attachment_added(self, attachment):
        if attachment.parent_realm == 'ticket':
            try:
                tn = TicketAttachmentNotifyEmail(self.env)
                # ticket that this attachment will modify
                ticket = Ticket(self.env, tkt_id=attachment.parent_id)
                tn.data.update({
                    'attachment': attachment,
                })
                tn.notify(ticket, newticket=False, modtime=attachment.date)
            except Exception, e:
                self.log.exception("Failure sending notification for attachment "
                                   "on ticket #%s: %s" % (ticket.id, exception_to_unicode(e)))

    def attachment_deleted(self, attachment):
        pass
