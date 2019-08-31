import requests
from urllib.parse import unquote    # convert HTML format to regular text
import sqlite3
import copy
from sys import stdout, stderr
import random

#import board   # WTF this doesn't work?

"""BOARD MODULE"""

import copy

#fout = open('debug.out', 'w')

class Board:
    def __init__(self, w, h, player0, player1, need):
        self.height = h
        self.width = w
        self.need = need

        self.board = []
        for i in range(w):
            self.board.append([' '] * h)

        self.pieces = [player0, player1]
        self.cur = 0
        self.npiece = 0

        self.mp = {player0 : 0, player1 : 1, ' ' : -1}

    def get_board(self):
        h, w = self.height, self.width
        res = []
        for i in range(h):
            res.append([' '] * w)

        for i in range(w):
            for j in range(h):
                res[j][i] = self.board[i][j]

        return res

    def string_version(self):
        attrs = ['height', 'width', 'need', 'board', 'pieces', 'cur', 'npiece', 'mp']
        res = ''
        for attr in attrs:
            res += str(getattr(self, attr)) + '\n'
        return res

    def horizontal(self, x, y):
        if not 0 <= x <= self.width - self.need:
            return -1

        if not 0 <= y < self.height:
            return -1

        c = self.board[x][y]
        if c == ' ':
            return -1

        poses = []
        for i in range(x, x + self.need):
            if self.board[i][y] != c:
                stderr.write('bad %d %d' % (i, y))
                return -1
            poses.append((i, y))

        return (self.mp[c], tuple(poses))

    def vertical(self, x, y):
        if not 0 <= x < self.width:
            return -1

        if not 0 <= y <= self.height - self.need:
            return -1

        c = self.board[x][y]
        if c == ' ':
            return -1

        poses = []
        for j in range(y, y + self.need):
            if self.board[x][j] != c:
                return -1
            poses.append((x, j))

        return (self.mp[c], tuple(poses))

    def maindiagonal(self, x, y):
        if not 0 <= x <= self.width - self.need:
            return -1

        if not 0 <= y <= self.height - self.need:
            return -1

        c = self.board[x][y]
        if c == ' ':
            return -1

        poses = []

        i, j = x, y
        while j < y + self.need:
            if self.board[i][j] != c:
                return -1
            poses.append((i, j))
            i += 1
            j += 1

        return (self.mp[c], tuple(poses))

    def antidiagonal(self, x, y):
        if not 0 <= x <= self.width - self.need:
            return -1

        if not self.need - 1 <= y < self.height:
            return -1

        c = self.board[x][y]
        if c == ' ':
            return -1

        poses = []

        i, j = x, y
        while i < x + self.need:
            if self.board[i][j] != c:
                return -1
            poses.append((i, j))
            i += 1
            j -= 1

        return (self.mp[c], tuple(poses))

    def status(self):
        has_space = False
        for i in range(self.width):
            for j in range(self.height):
                if self.board[i][j] == ' ':
                    has_space = True
                    break

            if has_space:
                break

        for i in range(self.width):
            for j in range(self.height):
                val = self.vertical(i, j)
                if val != -1:
                    return val

                val = self.horizontal(i, j)
                if val != -1:
                    return val

                val = self.maindiagonal(i, j)
                if val != -1:
                    return val

                val = self.antidiagonal(i, j)
                if val != -1:
                    return val

        return 'continue' if has_space else 'draw'

    def put(self, x, y):
        self.board[x][y] = self.pieces[self.cur]
        self.cur ^= 1
        #stderr.write('BOARD: ' + str(self.board))
        return self.status()


"""DATABASE STUFF"""

ttt_db = '__HOME__/ttt.db'
tttcomm_db = '__HOME__/tttcomm.db'
table_name = 'ttt_table'

def create_database():
    conn = sqlite3.connect(ttt_db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # make cursor into database (allows us to execute commands)
    ret = True
    try:
        c.execute('CREATE TABLE ' + table_name + '(gameid, board);') # run a CREATE TABLE command
        conn.commit()
        c.execute('INSERT into ' + table_name + ' VALUES (1,?);', (Board(3, 3, 'X', 'O', 3).string_version(), ))
        conn.commit()
    except:
        conn.commit() # commit commands
        things = c.execute('SELECT * FROM ' + table_name + ' WHERE gameid = 1').fetchall()
        if len(things) == 0:
            c.execute('INSERT into ' + table_name + ' VALUES (1,?);', (Board(3, 3, 'X', 'O', 3).string_version(), ))
            conn.commit()
        else:
            c.execute('UPDATE ' + table_name + ' SET board = ? WHERE gameid = 1', (Board(3, 3, 'X', 'O', 3).string_version(), ))
            conn.commit()


        things = c.execute('SELECT * FROM ' + table_name + ' WHERE gameid = 1').fetchall()
        assert(len(things) != 0)

        ret = False

    conn.commit() # commit commands
    conn.close() # close connection to database

    conn = sqlite3.connect(ttt_db)
    c = conn.cursor()
    conn.commit()
    conn.close()

    # try to do this also
    conn = sqlite3.connect(tttcomm_db)
    c = conn.cursor()
    try:
        c.execute('CREATE TABLE ' + table_name + '(gameid, who, putx, puty, end);') # run a CREATE TABLE command
    except:
        conn.commit()
        things = c.execute('SELECT * FROM ' + table_name + ' WHERE gameid = 1').fetchall()
        if len(things) == 0:
            c.execute('INSERT into ' + table_name + ' VALUES (?, ?, ?, ?, ?)', (1, -1, -1, -1, -1))
        else:
            c.execute('UPDATE ' + table_name + ' SET who = ?, putx = ?, puty = ?, end = ? WHERE gameid = 1', (-1, -1, -1, -1))

        things = c.execute('SELECT * FROM ' + table_name + ' WHERE gameid = 1').fetchall()
        assert(len(things) != 0)

    conn.commit()
    conn.close()
    return ret

def form_board(s):
    # s is the str
    cboard = Board(3, 3, 'X', 'O', 3)
    attrs = ['height', 'width', 'need', 'board', 'pieces', 'cur', 'npiece', 'mp']
    s = s.splitlines()
    assert(len(s) == 8)
    for i in range(8):
        setattr(cboard, attrs[i], eval(s[i]))
    return cboard

def get_board():
    conn = sqlite3.connect(ttt_db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # make cursor into database (allows us to execute commands)
    things = c.execute('SELECT * FROM ' + table_name + ' WHERE gameid = ?', (1, )).fetchall()
    conn.commit()
    assert(len(things))
    for t in things:
        t = t[1]
        stderr.write('things = ' + str(t))
        cboard = form_board(t)
        #stderr.write('--CURRENT BOARD STRING--')
        #stderr.write(cboard.string_version())
        return cboard
    assert(0)

def put(x, y):
    conn = sqlite3.connect(ttt_db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()
    cboard = get_board()
    if cboard.board[x][y] != ' ':
        conn.commit()
        conn.close()
        return None

    stderr.write('BEFORE call cboard.put: ' + str(cboard.get_board()))
    res = cboard.put(x, y)
    stderr.write('AFTER call cboard.put: ' + str(cboard.get_board()))
    c.execute('UPDATE ' + table_name + ' SET board = ? WHERE gameid = 1', (cboard.string_version(), ))
    conn.commit()
    conn.close()
    return res

def needs_another_player():
    conn = sqlite3.connect(ttt_db)
    c = conn.cursor()
    players = [i[1] for i in c.execute('SELECT * FROM ' + table_name + ' WHERE gameid = -1')]
    print(players)
    return len(players) != 0

def who_am_i():
    conn = sqlite3.connect(ttt_db)
    c = conn.cursor()
    players = None
    try:
        players = [i[1] for i in c.execute('SELECT * FROM ' + table_name + ' WHERE gameid = -1')]
    except sqlite3.OperationalError:
        create_database()
        players = [i[1] for i in c.execute('SELECT * FROM ' + table_name + ' WHERE gameid = -1')]

    if len(players) == 1:
        # then there is another player.
        c.execute('DELETE FROM ' + table_name + ' WHERE gameid = -1')
        conn.commit()
        nw = [i for i in c.execute('SELECT * FROM ' + table_name + ' WHERE gameid = -1')]
        assert len(nw) == 0
        res = ('X' if players[0] == 'O' else 'O')
        conn.commit()
        conn.close()
        return res
    else:
        me = 'XO'[random.randint(0, 1)]
        c.execute('INSERT into ' + table_name + ' values (-1, ?)', (me, ))
        conn.commit()
        nw = [i for i in c.execute('SELECT * FROM ' + table_name + ' WHERE gameid = -1')]
        assert len(nw) == 1
        conn.commit()
        conn.close()
        return me

def request_handler(request):
    request = request['form']

    if 'reset' in request:
        create_database()
        return

    if 'needs_another_player' in request:
        return str(needs_another_player())

    if 'who_am_i' in request:
        return who_am_i()

    if request['update'] == 'True':
        # this is the current player
        x = int(request['putx'])
        y = int(request['puty'])
        who = request['player']
        res = put(x, y)
        if res == None:
            return 'cannot put'

        # otherwise put it there successful!
        conn = sqlite3.connect(tttcomm_db)
        c = conn.cursor()

        if res == 'continue':
            c.execute('UPDATE ' + table_name + ' SET who = ?, putx = ?, puty = ?, end = ? WHERE gameid = 1', (who, x, y, 'False'))
            conn.commit()
            conn.close()
            return 'continue'
        elif res == 'draw':
            c.execute('UPDATE ' + table_name + ' SET who = ?, putx = ?, puty = ?, end = ? WHERE gameid = 1', (who, x, y, 'True'))
            conn.commit()
            conn.close()
 
            return 'draw'

        assert(isinstance(res, tuple))
        assert(len(res) == 2)
        assert(isinstance(res[1], tuple))

        # ok let's do this now
        # RETURNS
        spaces = str(res[0])
        for i in res[1]:
            spaces += ' ' + str(i)

        c.execute('UPDATE ' + table_name + ' SET who = ?, putx = ?, puty = ?, end = ? WHERE gameid = 1', (who, x, y, spaces))
        conn.commit()
        conn.close()

        return spaces

    # this is NOT the current player.
    conn = sqlite3.connect(tttcomm_db)
    c = conn.cursor()

    things = c.execute('SELECT * FROM ' + table_name + ' WHERE gameid = 1').fetchall()
    ret = ''
    who = request['player']
    thing = [thing for thing in things]
    assert(len(thing) == 1)
    thing = thing[0]
    # gameid, who, putx, puty, end
    if thing[1] == who:
        ret = '-1 -1 AND -1'
    else:
        ret = str(thing[2]) + ' ' + str(thing[3]) + ' AND ' + str(thing[4])
        c.execute('UPDATE ' + table_name + ' SET putx = ?, puty = ?, end = ? WHERE gameid = 1', (-1, -1, -1))

    conn.commit()
    conn.close()
    return ret
