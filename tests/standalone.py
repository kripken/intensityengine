
#=============================================================================
# Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
#
# This file is part of the Intensity Engine project,
#    http://www.intensityengine.com
#
# The Intensity Engine is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, version 3.
#
# The Intensity Engine is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with the Intensity Engine.  If not, see
#     http://www.gnu.org/licenses/
#     http://www.gnu.org/licenses/agpl-3.0.html
#=============================================================================


import os, math, signal, unittest, shutil, time, sys, random
import pexpect


MENU_DELAY = 0.4


def sign(x):
    if x < 0: return -1
    elif x > 0: return 1
    else: return 0

## TODO: Move to somewhere useful for the engine itself?
class Vector3:
    def __init__(self, x, y, z):
        self.x, self.y, self.z = x, y, z

    @staticmethod
    def parse(text):
        return Vector3(*(map(float, text.replace('>', '').replace('<', '').split(','))))

    def add(self, other):
        self.x += other.x
        self.y += other.y
        self.z += other.z
        return self

    def sub(self, other):
        self.x -= other.x
        self.y -= other.y
        self.z -= other.z
        return self

    def mul(self, other):
        self.x *= other
        self.y *= other
        self.z *= other
        return self

    def magnitude(self):
        return math.sqrt(self.x**2 + self.y**2 + self.z**2)

    def __str__(self):
        return '[%f,%f,%f]' % (self.x, self.y, self.z)

    def __repr__(self):
        return '[%f,%f,%f]' % (self.x, self.y, self.z)

    def copy(self):
        return Vector3(self.x, self.y, self.z)


barrier_counter = 0

class TestMaster(unittest.TestCase):
    # Utilities

    def assertExpect(self, proc, text, timeout=4):
        self.assertEquals(proc.expect(text, timeout), 0)

    ##! Make a new unique 'barrier' that can be read from a process, without it coming from any other source
    def make_barrier(self):
        global barrier_counter
        barrier_counter += 1
#        print "   Input barrier:", barrier_counter
        return 'okapi%dipako' % barrier_counter

    def make_unique(self):
        global barrier_counter
        barrier_counter += 1
        return 'xyz%d_zyx' % barrier_counter

    def inject_mouse_click(self, proc, x, y, button=1):
#        print "Inject mouse click", x, y, button

        barrier = self.make_barrier()
        proc.sendline('CModule.inject_mouse_position(%f, %f, True); CModule.flush_input_events(); print "%s"' % (x, y, barrier))
        self.assertExpect(proc, barrier)

        barrier = self.make_barrier()
        proc.sendline('CModule.inject_mouse_click(%d, 1); CModule.flush_input_events(); print "%s";' % (button, barrier))
        self.assertExpect(proc, barrier)

        barrier = self.make_barrier()
        proc.sendline('CModule.inject_mouse_click(%d, 0); CModule.flush_input_events(); print "%s";' % (button, barrier))
        self.assertExpect(proc, barrier)

#        print "   complete"

    def inject_key_press(self, proc, syms, _unicodes=None):
#        print "Inject key press", sym, _unicode

        if type(syms) not in [list, tuple]:
            syms = [syms]

        for i in range(len(syms)):
            sym = syms[i]
            if _unicodes is None:
                _unicode = sym
            else:
                _unicode = unicodes[i]

            barrier = self.make_barrier()
            proc.sendline('CModule.inject_key_press(%d, %d, 1, False)' % (sym, _unicode))
#            proc.sendline('CModule.flush_input_events(); print "%sk";' % barrier) # No unicode on way up
#            self.assertExpect(proc, barrier)
#
#            barrier = self.make_barrier()
            proc.sendline('CModule.inject_key_press(%d, 0, 0, False)' % sym)
            proc.sendline('CModule.flush_input_events(); print "%sk";' % barrier) # No unicode on way up
            self.assertExpect(proc, barrier)

#        print "   complete"

    # Setup/teardown

    def setUp(self):
        self.procs = []

        self.local_dir = '/dev/shm/intensityengine-temp-local'
        shutil.rmtree(self.local_dir, ignore_errors=True)
        os.makedirs(self.local_dir)

        self.master_dir = os.path.join(self.local_dir, 'master')
        os.makedirs(self.master_dir)
        shutil.copyfile(os.path.join('local', 'master_server', 'settings.cfg'), os.path.join(self.master_dir, 'settings.cfg'))
        shutil.copytree(os.path.join('local', 'master_server', 'templates'), os.path.join(self.master_dir, 'templates'))

        self.server_dir = os.path.join(self.local_dir, 'server')
        os.makedirs(self.server_dir)
        shutil.copyfile(os.path.join('local', 'server', 'settings.cfg'), os.path.join(self.server_dir, 'settings.cfg'))

        self.client_dir = os.path.join(self.local_dir, 'client')
        os.makedirs(self.client_dir)
        shutil.copyfile(os.path.join('local', 'client', 'settings_console.cfg'), os.path.join(self.client_dir, 'settings.cfg')) # console

    def tearDown(self):
        for proc in self.procs:
            try:
                os.kill(-proc.pid, signal.SIGKILL)
            except:
                try:
                    os.kill(proc.pid, signal.SIGKILL)
                except:
                    print "Warning: Killing failed for process", proc.pid
                    pass

#        shutil.rmtree(self.local_dir)

    def add_proc(self, proc):
        proc.delaybeforesend = 0.1 # Might need the default of 0.1 for the GUI - if you have problems, try that XXX
        self.procs.append(proc)
        return proc

    def run_command(self, procs, command, barrier=None):
        if barrier is None:
            barrier = self.make_barrier()
            command += ' ; print "%s"' % barrier

        if type(procs) not in [list, tuple]:
            procs = [procs]
        ret = []
        for proc in procs:
            unique = self.make_unique()
            proc.sendline('''

def doit_%s(): %s

main_actionqueue.add_action(doit_%s)

''' % (unique, command, unique))
            self.assertEquals(proc.expect(barrier, 4.0), 0)
            ret += [proc.readline().replace('\n', '').replace('\r', '')]
        if len(ret) == 1:
            return ret[0]
        else:
            return ret

    ## Flushes away the procedure's output
    def ignore_output(self, proc, text, timeout=1.0):
        try:
            proc.expect(text, timeout)
        except pexpect.TIMEOUT:
            pass
        except pexpect.EOF:
            pass

    def eval_script(self, proc, script):
        return self.run_command(proc, "CModule.run_script('log(WARNING, %s);', 'test'); print ''" % script, "WARNING]] - ")

    def run_script(self, proc, script):
        return self.run_command(proc, "CModule.run_script('%s;', 'test'); print 'alldone'" % script, 'alldone')

    def start_master(self):
        master = self.add_proc( pexpect.spawn('python intensity_master.py %s' % self.master_dir) )
        self.assertExpect(master, 'Would you like to create one now')
        master.sendline('no')
        self.assertEquals(master.expect("Create default user ('test')?", 4), 0)
        self.assertEquals(master.expect("Y/n]", 4), 0)
        master.sendline('')
        self.assertEquals(master.expect('Creating default user...', 4), 0)
        self.assertEquals(master.expect('Development server is running at http://127.0.0.1:8080/', 4), 0)

        return master

    def start_server(self):
        server = self.add_proc( pexpect.spawn('sh intensity_server.sh %s' % self.server_dir) )
        self.assertEquals(server.expect('recalculating geometry', 4), 0)
        self.assertEquals(server.expect('MAP LOADING]] - Success', 4), 0)

        # Check for downloaded assets
        self.assertTrue(os.path.exists(os.path.join(self.server_dir, 'packages', 'base', 'storming.tar.gz')))
        self.assertTrue(os.path.exists(os.path.join(self.server_dir, 'packages', 'base', 'storming', 'map.js')))
        self.assertTrue(os.path.exists(os.path.join(self.server_dir, 'packages', 'base', 'storming', 'map.ogz')))

        # Read some data, to be sure the map fully loaded, including entities
        output = self.eval_script(server, 'getEntity(50).position')
        self.assertTrue( Vector3.parse(output).sub(Vector3(347.85, 536.40, 392.10)).magnitude() < 0.03 )

        return server

    def start_client(self):
        client = self.add_proc( pexpect.spawn('sh intensity_client.sh %s' % self.client_dir) )# Use for debugging: , logfile=sys.stdout) )
        self.assertExpect(client, 'Starting threaded interactive console in parallel')
#        self.assertExpect(client, 'init: mainloop')

        time.sleep(0.5) # Might need to increase this

        # Log in
        self.inject_mouse_click(client, 0.479, 0.434, 1) # Select log in
        time.sleep(MENU_DELAY) # Let menu appear

        self.inject_mouse_click(client, 0.508, 0.474, 1) # Focus on username
        self.inject_key_press(client, [116, 101, 115, 116, 13]) # test

        self.inject_mouse_click(client, 0.439, 0.528, 1) # Focus on username
        self.inject_key_press(client, [115, 101, 99, 114, 101, 116, 13]) # secret

        self.inject_mouse_click(client, 0.210, 0.656, 1) # Do log in
        self.assertExpect(client, 'Logged in successfully')

        self.inject_mouse_click(client, 0.456, 0.454, 1) # Local connect

        # Map load
        self.assertExpect(client, 'MAP LOADING]] - Success', 8)
        self.ignore_output(client, 'physics for this round.', 3)
        self.ignore_output(client, '("start_red")', 3)

        # Check for downloaded assets
        self.assertTrue(os.path.exists(os.path.join(self.client_dir, 'packages', 'base', 'storming.tar.gz')))
        self.assertTrue(os.path.exists(os.path.join(self.client_dir, 'packages', 'base', 'storming', 'map.js')))
        self.assertTrue(os.path.exists(os.path.join(self.client_dir, 'packages', 'base', 'storming', 'map.ogz')))

#        # Read some data, to be sure the map fully loaded, including entities
        output = self.eval_script(client, 'getEntity(50).position')
        self.assertTrue( Vector3.parse(output).sub(Vector3(347.85, 536.40, 392.10)).magnitude() < 0.03 )

        return client

    def start_components(self):
        return self.start_master(), self.start_server(), self.start_client()

    def make_new_value(self, value):
        new_value = ''.join([str((int(v) + random.randrange(10)) % 10) for v in value])
        new_value = str(int(new_value)) # remove 0's from the beginning
        self.assertNotEquals(new_value, value)
        return new_value

    def testClient2Server(self):
        master, server, client = self.start_components()

        original_value = self.eval_script(server, 'getEntity(51).attr2')
        self.assertEquals(self.eval_script(client, 'getEntity(51).attr2'), original_value)
        new_value = self.make_new_value(original_value)

        self.run_script(client, 'getEntity(51).attr2 = %s' % new_value)
        time.sleep(0.25) # Let propagate
        self.assertEquals(self.eval_script([client, server], 'getEntity(51).attr2'), [new_value, new_value])

    def testServer2Client(self):
        master, server, client = self.start_components()

        original_value = self.eval_script(server, 'getEntity(51).attr2')
        self.assertEquals(self.eval_script(client, 'getEntity(51).attr2'), original_value)
        new_value = self.make_new_value(original_value)

        self.run_script(server, 'getEntity(51).attr2 = %s' % new_value)
        time.sleep(0.25) # Let propagate
        self.assertEquals(self.eval_script([client, server], 'getEntity(51).attr2'), [new_value, new_value])

    def testRestartMap(self):
        master, server, client = self.start_components()

        original_value = self.eval_script(server, 'getEntity(51).attr2')
        self.assertEquals(self.eval_script(client, 'getEntity(51).attr2'), original_value)
        new_value = self.make_new_value(original_value)

        self.run_script(server, 'getEntity(51).attr2 = %s' % new_value)
        time.sleep(0.25) # Let propagate
        self.assertEquals(self.eval_script([client, server], 'getEntity(51).attr2'), [new_value, new_value])

        self.inject_key_press(client, 27) # escape for menu
        self.inject_mouse_click(client, 0.417, 0.500) # restart map
        time.sleep(MENU_DELAY) # Let menu appear
        self.inject_mouse_click(client, 0.180, 0.605) # we are sure
        for proc in [server, client]:
            self.assertExpect(proc, 'MAP LOADING]] - Success', 8)

        self.run_command([client, server], 'time.sleep(1.0)') # flush messages

        self.assertEquals(self.eval_script([client, server], 'getEntity(51).attr2'), [original_value, original_value]) # Old value

    def testUploadMap(self):
        master, server, client = self.start_components()

        original_value = self.eval_script(server, 'getEntity(51).attr2')
        self.assertEquals(self.eval_script(client, 'getEntity(51).attr2'), original_value)
        new_value = self.make_new_value(original_value)

        self.run_script(server, 'getEntity(51).attr2 = %s' % new_value)
        time.sleep(0.25) # Let propagate
        self.assertEquals(self.eval_script([client, server], 'getEntity(51).attr2'), [new_value, new_value])

        self.inject_key_press(client, 27) # escape for menu
        self.inject_mouse_click(client, 0.414, 0.550) # upload map
        time.sleep(MENU_DELAY) # Let menu appear
        self.inject_mouse_click(client, 0.150, 0.592) # we are sure
        self.assertExpect(client, 'wrote map file', 4)

        for proc in [server, client]:
            self.assertExpect(proc, 'MAP LOADING]] - Success', 8)

        self.run_command([client, server], 'time.sleep(1.0)') # flush messages

        self.assertEquals(self.eval_script([client, server], 'getEntity(51).attr2'), [new_value, new_value]) # New value

    # Prevent regressions with state variable values flushing. That is, on the server,
    # wrapped C variables should be correctly initialized when assiged to, even before
    # the C entity is created (we queue and then flush them when the C entity is ready).
    def testSVFlushingRegressions(self):
        master, server = self.start_master(), self.start_server()

        self.run_script(server, 'deadlyArea = newEntity("DeadlyArea");')
        self.assertEquals(self.eval_script(server, 'deadlyArea.attr2'), '-1')
        self.assertEquals(self.eval_script(server, 'deadlyArea.collisionRadiusWidth'), '10')
        self.assertEquals(self.eval_script(server, 'deadlyArea.collisionRadiusHeight'), '10')

    # Ensure smooth movement of other clients
    def doTestSmoothMovement(self, fps, max_spread):
        master, server, client = self.start_components()

        if fps is not None:
            self.run_command(client, 'CModule.run_cubescript("maxfps %d")' % fps)

        self.run_script(server, 'bot = newNPC("Character");')
        bot_id = self.eval_script(server, 'bot.uniqueId')
        self.eval_script(server, 'bot.position.x = 512-15')
        self.eval_script(server, 'bot.position.y = 512-15')

        # Wait for bot to fall to floor
        time.sleep(1.0)
        pos = Vector3(0, 0, 0)
        while Vector3.parse(self.eval_script(server, 'bot.position')).sub(pos).magnitude() > 1:
            pos = Vector3.parse(self.eval_script(server, 'bot.position'))
            time.sleep(0.25)
        time.sleep(0.1)

        # Check client got it
        self.assertTrue(Vector3.parse(self.eval_script(client, 'getEntity(%s).position' % bot_id)).sub(pos) < 1)

        # Move bot and see that client smoothly tracks it
        newpos = pos.copy()
        move = Vector3(-10, 10, 0)
        newpos.add(move)
        direction = newpos.copy().sub(pos)
        self.run_script(server, 'bot.position = ' + str(newpos))
        smoothmove = 0.075 # Sync with sauer XXX
        delta = 0.001
#        print pos, newpos
        start = time.time()
        client.delaybeforesend = 0 # We need very responsive pexpect procs here!
        history = []
        while time.time() - start <= smoothmove*10:
            client_pos = Vector3.parse(self.eval_script(client, 'getEntity(%s).position' % bot_id))
#            print time.time() - start, client_pos
            history.append(client_pos)
            time.sleep(delta)

        # Validate the history

        # Start and finish
#        print pos, newpos, history
        self.assertTrue(history[0].copy().sub(pos).magnitude() < 1)
        self.assertTrue(history[-1].copy().sub(newpos).magnitude() < 1)

        # Steps are all in the right direction
        for i in range(len(history)-1):
            self.assertNotEquals( sign(history[i+1].x - history[i].x), -sign(direction.x) ) # Can be 0,
            self.assertNotEquals( sign(history[i+1].y - history[i].y), -sign(direction.y) ) # just not opposite

        # Steps are small
        for func in [lambda vec: vec.x, lambda vec: vec.y]:
            jumps = map(lambda i: abs(func(history[i])-func(history[i+1])), range(len(history)-1))
            spread = abs(func(history[0]) - func(history[-1]))
#            print jumps, spread, max(jumps)
            self.assertTrue(max(jumps) <= spread*max_spread)

    def testSmoothMovement30(self):
        self.doTestSmoothMovement(30, 0.666)

    def testSmoothMovementDefault(self):
        self.doTestSmoothMovement(None, 0.35)

    # Ensure smooth movement of other clients
    def testDeath(self):
        master, server, client = self.start_components()

        player_id = self.eval_script(client, 'getPlayerEntity().uniqueId')
        self.assertEquals(self.eval_script(client, 'getEntity(%s).animation' % player_id), '130')
        self.assertEquals(self.eval_script(client, 'getEntity(%s).health' % player_id), '100')

        self.run_script(client, 'getEntity(%s).health = 0' % player_id) # Die
        time.sleep(1.0)
        self.assertEquals(self.eval_script([client, server], 'getEntity(%s).health' % player_id), ['0', '0'])
        self.assertEquals(self.eval_script(client, 'getEntity(%s).animation' % player_id), '1')
        time.sleep(6.0) # Wait for respawn, and check that completely restored
        self.run_command(client, '\n') # Clean the output (comments on missing player start marker)
        self.assertEquals(self.eval_script([client, server], 'getEntity(%s).health' % player_id), ['100', '100'])
        self.assertEquals(self.eval_script(client, 'getEntity(%s).animation' % player_id), '130')

