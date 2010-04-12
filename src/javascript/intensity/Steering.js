
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

//
// The following steering system is inspired by Craig Reynolds' approach to the topic,
// http://www.red3d.com/cwr/steer/gdc99/
// and as implemeted in his OpenSteer library,
// http://opensteer.sourceforge.net/
// This is a JavaScript implementation of parts of that approach, and integrated with
// the Cube 2 engine where appropriate (for example, we use Sauer's collision
// querying and so forth, which leads to different ways to steer around obstacles, etc.).
// We try to use the same terminology as in OpenSteer, for clarity and interoperability
// possibilities in the future.
// 
// One significant difference with OpenSteer is that our actions are focused mainly
// on characters, i.e., figures walking on 2D surfaces, and not in general 3D space.
// Thus, our facing action deals mainly with yaws and so forth, and not general
// 3D velocities.
// 

//! How long it takes to 'simulate' a 'deep' 'thought' about how to steer.
SECONDS_TO_THINK_ABOUT_STEERING = 0.1;


//! Moves an angle towards another desired angle, at a specific speed. Makes sure
//! to normalize the angle relative to the desired angle, and to not overshoot so
//! it arrives exactly at the right angle.
//! @param angle The original angle, e.g., an entity's yaw.
//! @param desiredAngle The angle the entity wants to have, e.g., the yaw to some target
//! @param speed How fast the entity can turn towards the desired yaw, in degrees/second(?).
//! @return The modified angle.
function moveAngleTowards(angle, desiredAngle, speed) {
    angle = normalizeAngle(angle, desiredAngle);

    var originalSign = sign(desiredAngle - angle);
    angle += speed * originalSign;
    if (sign(desiredAngle - angle) === -originalSign && originalSign !== 0) { // We have overshot
        angle = desiredAngle;
    }

    return angle;
}


//! Faces an entity towards a target (or the _reverse direction from it). Changes yaw accordingly
//! for that entity.
//! @param actor The active entity, to perform the action.
//! @param target A target position towards which the actor will face.
//! @param speed How fast the entity can turn, in degrees.
//! @param _reverse Whether to face in the _reverse, i.e., away from the target (e.g., if fleeing from it).
//! Returns whether the facing operation is complete, i.e., now facing the target.
function faceTowards(actor, target, speed, _reverse) {
    var targetYaw = yawTo(actor.position, target, _reverse);
    actor.yaw = moveAngleTowards(actor.yaw, targetYaw, speed);

    var targetPitch = pitchTo(actor.position, target, _reverse);
    actor.pitch = moveAngleTowards(actor.pitch, targetPitch, speed)

    return Math.abs(actor.yaw - targetYaw) < 1.0 && Math.abs(actor.pitch - targetPitch) < 1.0;
}


//! Moves an entity towards another, both in facing and in walking towards it.
//! @param actor The active entity, to perform the action.
//! @param target A target entity towards which the actor will move.
//! @param _reverse Whether to face in the _reverse, i.e., away from the target (e.g., if fleeing from it).
//! @param facingSpeed How fast the entity can turn, in degrees/second(?).
//! Returns whether the operation is complete, i.e., now at the target and facing it.
function moveTowards(actor, target, facingSpeed, _reverse) {
    var facingComplete = faceTowards(actor, target, facingSpeed, _reverse);

    var distanceComplete = distance(actor.position, target) <= actor.radius*4;

    actor.move = 0; // By default, do not move forward

    if (!distanceComplete) {
        var targetYaw = yawTo(actor.position, target, _reverse);

        // Move forward, if facing closely enough
        if (Math.abs(targetYaw - actor.yaw) <= 30.0) {
            // Check if our current facing is reasonable in that moving forward will not hit something or fall us off a cliff

//            ahead_point = actor.position + actor.movement_speed*0.2*actor.velocity // Where we will soon be in 0.2 seconds

//            if distance(ahead_point, actor) < actor.radius/4:// or \
//               canWalkDistance(actor, (ahead_point, actor.radius)): // Too slow, to do on every movement!
            actor.move = +1; // We are facing closely enough to the target, and also can move forward without something bad happening,
                             // so go for it
        }
    }

    return (facingComplete && distanceComplete);
}


/*
//! Try to use a cached result for this actor and target, for a particular steering calculation
//! TODO: Maybe put this in util.py
function use_cached_result(actor, target, name):
    try:
        cached_value = getattr(actor, "__cached_" + name)
    except AttributeError:
        pass
    else:
        if cached_value['target_position'] == target.position and \
           distance(actor, cached_value['actor_position']) < actor.radius/2:
            return cached_value['result']

    return None

//! Store a cached result, see use_cached_result()
function set_cached_result(actor, target, name, value):
    cached_value = {}
    cached_value['target_position'] = target.position
    cached_value['actor_position'] = actor.position
    cached_value['result'] = value
    setattr(actor, "__cached_" + name, cached_value)
*/


//! Checks if there a straight line between an actor and target that can be followed to reach
//! that target. I.e., is it not blocked by obstacles. This does a few tests of line of sight, not just
//! one, for robustness.
//! @param actor The entity from which to test if the path is blocked.
//! @param target The target position which the actor wishes to reach.
//! @param cache_calculations Whether to cache results, so we don't check each frame, but only once in a while.
//! @return Whether the path is blocked.
function isPathBlocked(actor, target) { //, cache_calculations=true):
/*
    if cache_calculations:
        ret = use_cached_result(actor, target, "isPathBlocked")
        if ret is not None:
            return ret
*/
    var ray = target - actor.position;
    var mag = ray.magnitude();

    // Check for obstacles in our path
    var clearDistanceHead   = rayCollisionDistance(actor.position, ray);
    var clearDistanceMiddle = rayCollisionDistance(actor.position - new Vector3(0, 0, actor.eyeHeight/2), ray)

    var clearDistance = Math.max(clearDistanceHead, clearDistanceMiddle) // Need at least one to be ok, to 'see' the target

    var ret = clearDistance < (mag-actor.radius/4); // Whether it appears we can walk to the target without hitting something
/*
    if cache_calculations:
        set_cached_result(actor, target, "isPathBlocked", ret)
*/
    return ret;
}


//! Checks if an actor can walk forward a certain short distance without falling or hitting a non-climbable obstacle. Basically
//! we test small steps ahead to see if there isn't too big a jump in height, either up or down.
//! @param actor The entity for which to test if it can walk forward.
//! @param target A target entity towards which we test if it can walk.
//! @param radiiToCheck How many radii (of the entity) to look forward for. Default: 2
//! @param stepFraction We use small steps of size 1/this value for looking. Default: 3, so 1/3 radius per step.
//! @param cache_calculations Whether to cache results, so we don't check each frame, but only once in a while.
//! @return Whether a short distance can be walked without falling or hitting a non-climbable obstable.
function canWalkDistance(actor, target, radiiToCheck, stepFraction) { //, cache_calculations=true):
/*
    if cache_calculations:
        ret = use_cached_result(actor, target, "canWalkDistance")
        if ret is not None:
            return ret
*/
    radiiToCheck = defaultValue(readiiToCheck, 2);
    stepFraction = defaultValue(stepFraction, 3);

    // Check at increments of radius/3, since radius is about what we need to not fall
    var stepSize = actor.radius/stepFraction;
    var steps = radiiToCheck * stepFraction;

    var currPos = actor.position;

    var moveVec = target.subNew(actor.position);
    moveVec.normalize();
    moveVec.mul(stepSize);

    var fullHeight = actor.eyeHeight + actor.aboveEye;

    // How much of a floor difference we can accept per step. This can be going up or going down.
    var allowableDifferencePerStep = 2 * fullHeight * (stepSize/actor.radius); // Normalized relative to radius

    // Calculate the current floor distance carefully. Just a single point is not enough, since we might be overhanging a bit.
    // So try some points around as well. We take the highest floor, that is what we are perched upon
    lastFloorDist = highestFloorDistance(currPos, 2.*fullHeight, actor.radius);

    var ret = true; // Unless there is a problem, all is ok

    // Take steps, checking in each
    for (var i = 0; i < steps; i++) {
        currPos.add(moveVec);

        // Check for floor. We take the highest, i.e., deepest floor, the maximum, as we want to avoid overhanging
        var floorDist = highestFloorDistance(currPos, lastFloorDist + fullHeight/2, actor.radius);

        if (Math.abs(floorDist - lastFloorDist) > allowableDifferencePerStep) {
            ret = false;
            break;
        }

        lastFloorDist = floorDist;
    }

/*
    if cache_calculations:
        set_cached_result(actor, target, "canWalkDistance", ret)
*/
    return ret;
}


//! Makes the actor of the action turn towards a target, and stop when facing it.
FaceTowardsAction = TargetedAction.extend({
    _name: 'FaceTowardsAction',

    create: function(target, kwargs) {
        this._super(target, kwargs);

        if (this.animation !== ANIM_IDLE | ANIM_LOOP) {
            this.animation = ANIM_IDLE | ANIM_LOOP; // Set only if changed, to save bandwidth
        }
    },

    doExecute: function(seconds, _reverse) {
        _reverse = defaultValue(_reverse, false);

        if (this.target.deactivated) {
            return true;
        }

        return faceTowards(this.actor, this.target, seconds*this.actor.facingSpeed, _reverse);
    }
});


//! Move towards a target. The target may be moving or not, and just needs to supply an attribute with (x,y,z).
// TODO: Perhaps save the last distance, and if we start getting farther then give up, or only turn for a while?
// This might happen if we are quite close and shoot past the target as we spin. But if the target is moving,
// then this is not so simple.
ArrivalAction = TargetedAction.extend({
    _name: 'ArrivalAction',

    create: function(kwargs) {

        this._super(kwargs);

        this.desiredDistance = kwargs.desiredDistance;

        this.quitIfBlocked = defaultValue(kwargs.quitIfBlocked, true);

        if (this.animation !== ANIM_IDLE | ANIM_LOOP) {
            this.animation = ANIM_IDLE | ANIM_LOOP // Set only if changed, to save bandwidth
        }
    },

    doExecute: function(seconds, forceIndex) {
        if (this.quitIfBlocked) {
            if (this.actor.blocked) {
                return true;
            }
        }

        if (target.deactivated) {
            return true;
        }

        return moveTowards(this.actor, this.target.position, seconds*this.actor.facingSpeed, _reverse);
    },

/*
        // If our target has vanished from the game, then stop following
        if this.targetQueue[0].deactivated:
            return true

        if not this.timer.tick(seconds):
            found = true // We just go to the last target - no thinking this time
        else:
            // Test if we have a clear line to ANY of our targets, most important first, and move towards that one
            found = False

            // Search entire queue if not given directions which to check out
            if forceIndex is None:
                valid_range = range( len(this.targetQueue) )
            else:
                valid_range = [forceIndex]

            for i in valid_range:
                curr_target = this.targetQueue[i]

                // See if we can both walk a little forward, and have a direct line of walking further ahead, to the current target
                if canWalkDistance(this.actor, curr_target) and not isPathBlocked(this.actor, curr_target):
                    this.targetQueue = this.targetQueue[:i+1] // Discard all further targets
                    found = true
                    break

        // If we didn't find a target, go to where we last saw the target - maybe once we get there it's position will become clear
        // (maybe it's just around the corner).
        if not found and this.last_valid_target_position is not None:
//            print "Trying last valid position:", this.last_valid_target_position

            this.targetQueue.append( WorldObject(this.last_valid_target_position, this.actor.radius/2) )
            this.last_valid_target_position = None // Can't use this anymore
            // Recurse a call. This will make us check whether in fact this new position can be walked to, isn't blocked, etc.
            // We tell ourselves explicitly to not waste time on the earlier unreachable indexes.
            // We also have wasted a little time in thinking, so consider that.
            if seconds >= SECONDS_TO_THINK_ABOUT_STEERING:
                return this.doExecute(seconds - SECONDS_TO_THINK_ABOUT_STEERING, forceIndex = len(this.targetQueue)-1)
            else:
                return False // Will try to do this next time

        // If still not found, try to at least walk a little forward if possible, to the last target, or a prior one if none is
        // available
        if not found:
            for i in range(len(this.targetQueue)-1, 0, -1):
                if canWalkDistance(this.actor, this.targetQueue[i]):
                    this.targetQueue = this.targetQueue[:i+1] // Discard all further targets
                    found = true
                    break

        // If we can't even walk a little forward to any of our goals, then try to walk to a random location with reasonable
        // yaw to the current target - perhaps this way we will get around any obstacles?
        if not found:
            currPosition = this.actor.position

            // Ensure at least one iteration in the next loop
            seconds = Math.max(seconds, SECONDS_TO_THINK_ABOUT_STEERING)

            // Keep trying until we run out of time
            while seconds >= SECONDS_TO_THINK_ABOUT_STEERING and not found:
                seconds -= SECONDS_TO_THINK_ABOUT_STEERING

                if random.uniform(0., 1.) < 0.1: // With low chance, try a simple random position TODO: make not == 0
                    // TODO: Move in a reasonable yaw, not at random
                    random_radius = this.actor.radius * random.uniform(0, 4.5)

                    new_target_position = currPosition + new Vector3( random.uniform(-random_radius, random_radius), \
                                                                   random.uniform(-random_radius, random_radius), \
                                                                   0 )
//                    print "RU:", new_target_position

                else:
                    targetYaw = yawTo(this.actor, this.target)
                    yaw_sigma = Math.max(5*(this.failures/2 + 1), 30)
                    random_yaw = targetYaw + random.gauss(0, yaw_sigma) // extend sigma as more failures accumulate

                    random_radius = this.actor.radius * random.uniform(3, 4)

                    new_target_position = currPosition + new Vector3( sin(radians(random_yaw)) * random_radius, \
                                                                   -cos(radians(random_yaw)) * random_radius, \
                                                                   0 )

//                    print "TY:", targetYaw, yawTo(this.actor, new_target_position), "(", random_yaw, ")"

//                print "Trying new random location:", new_target_position

                if canWalkDistance(this.actor, new_target_position):
                    this.targetQueue.append(new_target_position)
                    found = true

        // If we found a place to go, go there
        if found:
            this.failures = 0

            curr_reachable_target = this.targetQueue[-1]

            // Remember this position, if it is the actual target we are tracking, i.e. the target queue has but one target. We
            // might use it later. We make a copy, as we don't want to point to a changing position
            if len(this.targetQueue) == 1:
                this.last_valid_target_position = curr_reachable_target.position.copy()

            // We can see reach a current target, move straight to it
            reached = moveTowards(this.actor, curr_reachable_target, seconds*this.actor.facingSpeed, false) // No _reverse for now, TODO

            if reached:
                this.targetQueue.pop()
                return len(this.targetQueue) == 0 // We are done if there are no more queued targets
            else:
                return False // Keep moving towards current target next time

        else:
            this.failures += 1
            return False
*/
    doFinish: function() {
        this.actor.move = 0 // Stop moving - important for PC, NPCs would do this anyhow
    }
});


/*
//! A form of Arrival (or Pursuit) that attempts to go where the target *will* be. Instead of normal
//! prediction of future target locations, we use the guideline hinted at in the CR's Steering paper,
//! of maintaining a constant heading being one way to optimize pursuit. FIXME: This is buggy, nonusable for now.
class PredictivePursuitAction(TargetedAction):
    function __init__(this, *args, **kwargs):
        TargetedAction.__init__(this, *args, **kwargs)

        this.old_targetYaw = 0.

    function doExecute(this, seconds, _reverse=False):
        new_targetYaw = yawTo(this.actor.position, this.target.position)

        if _reverse: // If _reverse, then try to face *away* from the target
            new_targetYaw += 180

        if Math.abs(new_targetYaw - this.actor.yaw) <= 33:
            this.actor.move = +1 // We are facing closely enough, move towards it
        else:
            this.actor.move = 0

        // Try to keep yaw towards the target constant
        desired_yaw = this.actor.yaw - 10.*sign(new_targetYaw - this.old_targetYaw)

        this.actor.yaw = moveAngleTowards(this.actor.yaw, desired_yaw, seconds*this.actor.facingSpeed)

        // Save target yaw
        this.old_targetYaw = new_targetYaw

        return False // Never stop

    function doFinish(this):
        this.actor.move = 0
*/


//! Flee from a target.
FleeAction = FaceTowardsAction.extend({
    _name: 'FleeAction',

    doExecute: function(seconds) {
        // Start by facing away from
        this._super(seconds, true);

        if (Math.abs(this.targetYaw - this.actor.yaw) <= 90) {
            this.actor.move = +1; // Move forward to get away
        } else {
            this.actor.move = -1; // Move backward to get away
        }

        return false; // Keep at it, never stop fleeing until forced to
    },

    doFinish: function() {
        this.actor.move = 0;
    }
});


// Public interface for some 'internal' functions

Steering = {
    faceTowards: faceTowards
};

