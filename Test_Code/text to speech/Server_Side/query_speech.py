'''Calculation stuff.'''

import datetime
from fractions import gcd

'''Database stuff.'''

import requests
from urllib.parse import unquote    # convert HTML format to regular text
import sqlite3
# from gestures_calc import cross_correlation

messages_db = '__HOME__/messages.db'

def create_database():
    conn = sqlite3.connect(messages_db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # make cursor into database (allows us to execute commands)
    try:
        c.execute('''CREATE TABLE message_table (timestamp, user, message);''') # run a CREATE TABLE command
    except:
        pass
    conn.commit() # commit commands
    conn.close() # close connection to database

def lookup_database():
    conn = sqlite3.connect(messages_db)
    c = conn.cursor()
    things = c.execute('''SELECT * FROM message_table;''').fetchall()
    #print('All in message_table: ' + str(things))   # iterate through things
    things = c.execute('''SELECT * FROM message_table ORDER BY timestamp DESC LIMIT 3;''').fetchall()
    things = [i for i in reversed(things)]
    #print('things:')
    #for i in things:
    #    print(i)
    
    # ok this is the thing

    conn.commit()
    conn.close()
    return things

def update_database(user, msg):
    #print('ADDING %s to database' % msg)
    conn = sqlite3.connect(messages_db)
    c = conn.cursor()

    c.execute('''INSERT into message_table VALUES (?, ?, ?);''', (datetime.datetime.now(), user, msg))

    conn.commit()
    conn.close()

def request_handler(request):
    #print('request = ' + str(request))
    request = request['form']
    create_database()

    # ok let's get the messages
    if 'message' in request and 'user' in request:
        msg = request['message']
        user = request['user']
        #print('Storing message!')
        #print('msg = ' + msg + ', user = ' + user)
        update_database(user, msg)
        #if not lookup_database():
        #    raise AssertionError('EMPTY DATABASE!!!')
        #else:
        #    print('lookup_database is %s' % str(lookup_database()))
        #print('DATABASE ' + str(lookup_database()))
        return 'MESSAGE: ' + msg
    else:
        #print('Query for top 3')
        # then it's a query for the top 3
        res = '@MESSAGES: '
        things = lookup_database()
        for thing in things:
            # timestamp, user, message
            user, message = thing[1], thing[2]
            res += user + ': ' + message + '\n\n'
        return res

#    if 'no_gesture_exists_called' in request:
#        # then determine whether no gesture exists
#        if 'id' in request:
#            return has_gesture(int(request['id']))
#        return '@`NOGESTURE: ' + str(not has_gesture(0) and not has_gesture(1) and not has_gesture(2))
#
#    data = request['data']
#    if request['result'] == 'False':
#        if data != '[]':
#            i = int(request['id'])
#            update_database(i, data)
#        return 'FINISHED UPDATING'
#    elif request['result'] == 'True':
#        data = eval(data)
#        #return '@`RESULT: ' + '0 ' + str(data) + ' ' + str(lookup_database(0))
#        #assert(len(data))
#        #print('data = ' + str(data))
#        return '@`RESULT: ' + str(most_similar(data))
#    else:
#        raise ValueError('invalid value for request[\'result\']')
