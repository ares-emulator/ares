const {test}=require('node:test');
const checkSignals=require('./signal-instance-check.cjs');
test('actual app signals, named restart, fresh authentication and interrupted-command nonreplay', {timeout:30000}, async()=>{await checkSignals()});
