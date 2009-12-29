// Simple inheritance system, due to John Resig
// http://ejohn.org/blog/simple-javascript-inheritance/
// Thanks!

assert( "typeof Class === 'function'" );

assert( "typeof Class.extend === 'function'" );

var Person = Class.extend({
 create: function(isDancing){
   this.dancing = isDancing;
 },
 dance: function(){
   return this.dancing;
 }
});

var Ninja = Person.extend({
  create: function(){
    this._super( false );
  },
  dance: function(){
    return this._super();
  },
  swingSword: function(){
    return true;
  }
});

var p = new Person(true);
var n = new Ninja();

assert( "p.dance() === true" );
assert( "n.dance() === false" );
assert( "n.swingSword() === true" );
assert( "p instanceof Person" );
assert( "p instanceof Class" );
assert( "n instanceof Ninja" );
assert( "n instanceof Person" );
assert( "n instanceof Class" );

