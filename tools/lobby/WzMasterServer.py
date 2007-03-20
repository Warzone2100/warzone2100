#!/usr/bin/python -t

__author__  = "Christian Vest Hansen aka. karmazilla"
__version__ = "0.0.1"
__license__ = "GPL"

defaultPort = 2620
defaultDb = "WzMasterServer.db"

# automatically delete games older than this time amount in milliseconds
DB_CLEANUP_DELAY = 1000*60*60*24

"""
TODO:
	+ Automatic DB clean-up & pruning thread.
	+ Look into that "VACUUM" thingy in SQLite... Might be important.
	+ OR make it connect to a MySQL database instead.
	+ More flexible listing and seaching in the games DB?
	+ Documentation on how to invoke the service.
"""

STATE_OPEN = 1
STATE_STARTED = 2
STATE_CLOSED = 3

# check & import our dependant packages...
import sys
import time
try:
	from optparse import OptionParser
except:
	print """Missing dependency: OptionParser module.
	This module is part of a standard Python distribution.
	You Python installation may be broken."""
	sys.exit(1)
try:
	# we don't actually need this, however, JSON-RPC does, so we check it
	import simplejson
except:
	print """Missing dependency: simplejson module.
	If you are on a Debian/Ubuntu based system, you should
	be able to install this module with:
	sudo apt-get install python-simplejson"""
	sys.exit(1)
try:
	from jsonrpc.socketserver import ThreadedTCPServiceServer
except:
	print """Missing dependency: JSON-RPC module.
	Please install this module by following this guide:
	http://json-rpc.org/wiki/python-json-rpc#Installation"""
	sys.exit(1)
try:
	from pysqlite2 import dbapi2 as sqlite
except:
	print """Missing dependency: pysqlite2 module.
	If you are a Debian/Ubuntu based system, you should
	be able to install this module with:
	sudo apt-get install python-pysqlite2"""
	sys.exit(1)

def timestamp():
	return long(time.time() * 1000)

class Service:
	def __init__(self, connection):
		self.db_name = connection
	
	def openGame(self, host, port, name, version, maxplayers, skmap, techlevel):
		connection = sqlite.connect(self.db_name)
		connection.cursor().execute("INSERT INTO games VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
			(host, port, name, version, maxplayers, skmap, techlevel, STATE_OPEN, timestamp())
		)
		connection.commit()
		return "Done"
	
	def startGame(self, host, port):
		connection = sqlite.connect(self.db_name)
		connection.cursor().execute("UPDATE games SET state = ? WHERE host = ? AND port = ?",
			(STATE_STARTED, host, port)
		)
		connection.commit()
		return "Done"
	
	def closeGame(self, host, port):
		connection = sqlite.connect(self.db_name)
		connection.cursor().execute("UPDATE games SET state = ? WHERE host = ? AND port = ?",
			(STATE_CLOSED, host, port)
		)
		connection.commit()
		return "Done"
	
	def listGames(self, byState):
		state = 0
		if byState == "open":
			state = STATE_OPEN
		elif byState == "closed":
			state = STATE_CLOSED
		elif byState == "started":
			state = STATE_STARTED
		if state == 0:
			raise InvalidMethodParameters, "Parameter for listGames must be one of 'open', 'started' or 'closed'."
		try:
			connection = sqlite.connect(self.db_name)
			cursor = connection.cursor()
			cursor.execute("SELECT * FROM games WHERE state = " + str(state))
			result = cursor.fetchall()
			return result
		except Exception, details:
			print "Caught exception:", details
			return "Caught exception: " + details


def setup_db(db_name):
	connection = sqlite.connect(db_name)
	dbcursor = connection.cursor()
	try:
		dbcursor.execute("""
		CREATE TABLE games (
			host TEXT,
			port INTEGER,
			name TEXT,
			version TEXT,
			maxplayers INTEGER,
			map TEXT,
			techlevel INTEGER,
			state INTEGER,
			timestamp INTEGER
		)""")
		connection.commit()
		print "Datebase created."
	except:
		print "Error creating database table. Maybe it already existed."
		connection.rollback()

def listen_forever(port, connection):
	print "Starting to listen on port", port
	try:
		ThreadedTCPServiceServer(Service(connection)).serve(('localhost', port))
	except KeyboardInterrupt:
		print "Keyboard interrupt. Stopping service."
#	except ExitInterrupt:
#		print "Caught termination signal. Stopping service."

def main():
	print "Warzone2100 Master Server (JSON-RPC) v.", __version__, "by", __author__
	parser = OptionParser()
	parser.add_option("-p", "--port", dest="port", help="The port the master server should listen on.", metavar="PORT")
	parser.add_option("-d", "--database", dest="db",
	help="""The name of the SQLite database file to use as backing datastore, or \":memmory:\" for transient storage.""", metavar="FILE.db")
	(options, args) = parser.parse_args()
	
	dbstore = options.db
	if dbstore == None:
		print "No database file specified. Defaulting to", defaultDb
		dbstore = defaultDb
	setup_db(dbstore)
	
	port = options.port
	if port == None:
		print "No port specified. Defaulting to listen on port", defaultPort
		port = defaultPort
	listen_forever(2620, dbstore)

if __name__=='__main__':
	main()