import sqlite3
import datetime

img_db="__HOME__/images.db"

def request_handler(request):
    conn=sqlite3.connect(img_db)
    c=conn.cursor()
    if request["method"]=="POST":
        user=request['form']['user']
        message=request['form']['message']
        c.execute('''CREATE TABLE IF NOT EXISTS test_table (user,message,time);''')
        c.execute('''INSERT into test_table VALUES (?,?,?);''',(user,message,datetime.datetime.now())) #with time
        conn.commit()
        conn.close()
        return "image sent"
    if request['method']=="GET":
        index=int(request['values']['index'])
        things = c.execute('''SELECT * FROM test_table;''').fetchall()
        oppo_user=None
        if request['values']['user']=='A':
            oppo_user='B'
        else:
            oppo_user='A'
        count=0
        for i in range(len(things)-1,-1,-1):
            if things[i][0]==oppo_user:
                if count==index:
                    return (things[i][1])
                count+=1
        return "no image from "+oppo_user
# 1977 account july 18 2017
# james 51674
