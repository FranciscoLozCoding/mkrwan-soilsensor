var LED_STATES = ['off', 'on']

function decodeUplink(input) {
  var bytes = input['bytes'];
  var i = 0;
  var decoded = {
    measurements:[]
  }
  var temp;
  var moist;
  
  while(i < bytes.length){
    
 	s_ch = bytes[i]; 	//Channel
	s_type = bytes[i+1];	//Message Type
	s_size = 8; //Data length\
    s_hb = bytes[i+2];
    s_lb = bytes[i+3];
    
    switch (s_type) {
      case 103: //temperature
        temp = (((s_hb<<8) + s_lb )/100);
        break;
      case 104: //soil moist
        moist = (((s_hb<<8) + s_lb )/100);
        break;
    }
    i += 4;
    
  }
  
  // multiply by 1000 to convert the analog read to mV
  temp = temp*1000;
  moist = moist*1000;
  
  //follow payload format for lorawan-listener
  decoded.measurements.push({
    name: "lorawan.soiltemp",
    value: temp
    });
  
  decoded.measurements.push({
    name: "lorawan.soilmoist",
    value: moist
    });
  
  return {data: decoded};
}

function encodeDownlink(input) {
  var i = LED_STATES.indexOf(input.data.ledState);
  if (i === -1) {
    return {
      errors: ['unknown led state'],
    };
  }
  return {
    bytes: [i],
    fPort: 1,
  };
}

function decodeDownlink(input) {
  return {
    data: {
      ledState: LED_STATES[input.bytes[0]]
    }
  }
}