import copy

class Board:
    def __init__(self, h, w, player0, player1, need):
        self.height = h
        self.width = w
        self.need = need

        self.board = []
        for i in range(h):
            self.board.append([' '] * w)

        self.pieces = [player0, player1]
        self.cur = 0
        self.npiece = 0

        self.mp = {player0 : 0, player1 : 1, ' ' : -1}

    def vertical(self, x, y):
        if not 0 <= x < self.height - self.need:
            raise IndexError('x out of range')

        if not 0 <= y < self.width:
            raise IndexError('y out of range')

        c = self.board[x][y]
        if c == ' ':
            return -1

        poses = []
        for i in range(x, x + need):
            if self.board[i][y] != c:
                return -1
            poses.append((i, y))

        return (self.mp[c], tuple(poses))

    def horizontal(self, x, y):
        if not 0 <= x < self.height:
            raise IndexError('x out of range')

        if not 0 <= y < self.width - self.need:
            raise IndexError('y out of range')

        c = self.board[x][y]
        if c == ' ':
            return -1

        poses = []
        for j in range(y, y + need):
            if self.board[x][j] != c:
                return -1
            poses.append((x, j))

        return (self.mp[c], tuple(poses))

    def maindiagonal(self, x, y):
        if not 0 <= x < self.height - need:
            raise IndexError('x out of range')

        if not 0 <= y < self.width - need:
            raise IndexError('y out of range')

        c = self.board[x][y]
        if c == ' ':
            return -1

        poses = []

        i, j = x, y
        while j < y + need:
            if self.board[i][j] != c:
                return -1
            poses.append((i, j))
            i += 1
            j += 1

        return (self.mp[c], tuple(poses))

    def antidiagonal(self, x, y):
        if not 0 <= x < self.height - need:
            raise IndexError('x out of range')

        if not need - 1 <= y < self.width:
            raise IndexError('y out of range')

        c = self.board[x][y]
        if c == ' ':
            return -1

        poses = []

        i, j = x, y
        while i < x + need:
            if self.board[i][j] != c:
                return -1
            poses.append((i, j))
            i += 1
            j -= 1

        return (self.mp[c], tuple(poses))

    def status(self):
        has_space = False
        for i in range(self.height):
            for j in range(self.width):
                if self.board[i][j] == ' ':
                    has_space = True
                    break

            if has_space:
                break

        if not has_space:
            return 'draw'

        for i in range(self.height):
            for j in range(self.width):
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

        return 'continue'

    def put(self, x, y):
        self.board[x][y] = self.pieces[self.cur]
        self.cur ^= 1
        return self.status()
