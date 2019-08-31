import requests
import sqlite3
import datetime

power_db = '__HOME__/power.db'

c = None
datetime_format = '%Y-%m-%d %H:%M:%S.%f'

def get_name(name):
    global c
    val = list(c.execute('SELECT * FROM power_table WHERE name = ?;', (name, )))
    if len(val) == 0:
        return None
    else:
        assert len(val) == 1
        return datetime.datetime.strptime(val[0][1], datetime_format)

def request_handler(request):
    conn = sqlite3.connect(power_db)
    global c
    c = conn.cursor()

    op = request['form' if 'form' in request else 'values']['op']
    ret = ''
    if op == 'first':
        c.execute('CREATE TABLE IF NOT EXISTS power_table (name, time);')
        now = datetime.datetime.now()
        for name in ['first', 'latest']:
            if get_name(name) != None:
                c.execute('UPDATE power_table SET time = ? WHERE name = ?;', (now, name))
            else:
                c.execute('INSERT INTO power_table VALUES (?, ?);', (name, now))
        ret = 'first = ' + str(now)
    elif op == 'get':
        first = get_name('first')
        latest = get_name('latest')
        #ret += str((things[1][1] - things[0][1]).total_seconds()) + ' seconds'
        ret = 'first = %s\nlatest = %s\ndiff = %s sec\n' % (first, latest, (latest - first).seconds)
    elif op == 'put':
        # push new time
        c.execute('')
        now = datetime.datetime.now()
        c.execute('UPDATE power_table SET time = ? WHERE name = ?', (now, 'latest'))
        ret = 'latest = %s\ndiff = %s sec\n' % (now, (now - get_name('first')).seconds)
    else:
        raise ValueError('not valid operation')

    conn.commit()
    conn.close()
    return ret
