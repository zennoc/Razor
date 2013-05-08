var util = require('util');

var safeArgPattern = /^[^\x00-\x1f&\|]+$/;
var safeArgMaxLength = 200;

/**
 * Abstract error class. Needed to get the constructor stuff right. Can also
 * be used for generic trap of all sub-classes.
 */
var AbstractError = function(msg, constructor)  {
    Error.captureStackTrace(this, constructor);
    this.message = msg;
}
util.inherits(AbstractError, Error);

/**
 * Error thrown when we receive URI paths that doesn't pass our validation checks.
 */
var InvalidURIPathError = function(msg) {
	InvalidURIPathError.super_.call(this, msg, this.constructor);
}
util.inherits(InvalidURIPathError, AbstractError);

/**
 * Perform URI decoding on the given argument and assert that it
 * matches the regular expression that defines what we consider safe
 * @param arg The argument to decode and check
 * @returns The decoded arg
 * @throws An 'Illegal path component' if the arg is considered unsafe
 */
exports.urlDecode = function(arg) {
    arg = decodeURIComponent(arg);
    if(!(arg.length <= safeArgMaxLength && arg.indexOf('..') < 0 && safeArgPattern.test(arg)))
        throw new InvalidURIPathError("Illegal path component: '" + arg + "'");
    return arg;
}

/**
 * Respond with an error code based on a trapped exception. 
 * @param res The response object
 * @param e The error object
 */
exports.returnError = function(res, e) {
	if(e === undefined || e == null) {
        console.error('Error 500: Exception with no furter info');
        res.send('Internal error', 500);
	} else if(e instanceof InvalidURIPathError || e instanceof URIError) {
        console.error('Error 404: ' + e.message);
        res.send(e.message, 404);
    } else if(e instanceof Error){
        console.error('Error 500: ' + e.message);
        res.send(e.message, 500);
    } else {
        // Assume that e can be converted into a string
        console.error('Error 500: ' + e);
        res.send('' + e, 500);
    }
}

exports.AbstractError = AbstractError;
exports.InvalidURIPathError = InvalidURIPathError;
