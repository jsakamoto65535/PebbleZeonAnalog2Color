//
function fetchWeather(realPlace, displayPlace, latitude, longitude) {
  console.log("fetchWeather call!!");
  var req = new XMLHttpRequest();
  // 移動経度が指定された場合は緯度経度で検索、そうでなければ名前で検索 
  if(latitude == 0 && longitude ==0)
    {
     req.open('GET', "http://api.openweathermap.org/data/2.5/weather?q=" +realPlace+"&units=metric"+"&appid=f90217265bbcaa4e92ff5d6363950111", true);
    }
  else
    {
     req.open('GET', "http://api.openweathermap.org/data/2.5/weather?units=metric" +"&lat=" + latitude + "&lon=" + longitude + "&cnt=1"+"&appid=f90217265bbcaa4e92ff5d6363950111", true);      
    }
   
  req.onload = function(e) {
    if (req.readyState == 4) {
      if(req.status == 200) {
         
        var response = JSON.parse(req.responseText);
 //       console.log("JSON Parsed:" + response.weather[0]);
        
        if (response && response.weather && response.weather.length > 0) {          
          console.log("fetchWeather success!!");

          var weather = response.weather[0].id.toString();
// org code
          var temp = response.weather[0].main.toString() + "," + response.main.temp.toString() + "°C";
//          var intpart = response.main.temp.toString().split('.');
//          var temp = response.main.temp.toString()+"°C";
          
          console.log("Receive Response weather = " + response.weather[0].main.toString() );         
          console.log("Receive Response temp = " + response.main.temp.toString());
          Pebble.sendAppMessage({
             "weather": weather,
            "temp":temp

          });
          console.log("Receive Response Seuccess!!");
        }
        else
        {
            console.log("Response is Error");            
        }
 
      } else {
        console.log("Error");
      }
    }
  };
   
  req.send(null);
}
 
/*
 * getCurrentPositionが成功したときの処理
 */
function locationSuccess(pos) {
  console.log("Location success");
  var coordinates = pos.coords;
  var displayName = "la"+coordinates.latitude.toString().slice(0,3) + 
                  ",lo"+ coordinates.longitude.toString().slice(0,3);
  fetchWeather("",displayName, coordinates.latitude, coordinates.longitude);
}
 
/*
 * getCurrentPositionが失敗したときの処理
 */
function locationError(err) {
  console.log("Location error");
  Pebble.sendAppMessage({
    "weather": "Jamming...Req Failed"
   });
}
 
// getCurrentPositionにオプションの設定
var locationOptions = { "timeout": 15000, "maximumAge": 60000 }; 

function messageProc(e)
{
  console.log("appmessage");
  
  window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);

/*  
  // 0 - 3まで
  var rand = Math.floor( Math.random() * 4 ) ;
  
  if (rand == 0)  // New Yark
    {     
      fetchWeather("NewYork,us","NewYark", 0, 0);
    }else if(rand ==1) // Great Canion
    {
      fetchWeather("GrandCanion,us","GreatCanion", 0,0 );
    }else if(rand ==2) // Great Canion
    {
      fetchWeather("TaklamakanDesert,cn","Taklamakan", 0,0 );
    }else{  // This Place
      window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
    }
    */
}


// Pebbleのイベント appmessage が発生した時の処理を登録
Pebble.addEventListener("appmessage",messageProc);

