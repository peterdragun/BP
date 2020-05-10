import React from 'react';
import {
  Page,
  Navbar,
  List,
  Link,
  Button,
  Popup,
  NavRight,
  Block,
  ListItem,
  ListInput
} from 'framework7-react';

export default class extends React.Component {
  constructor(props) {
    super(props);

    this.state = {
      popupTitle: "Error",
      popupOpened: false,
      message: "",
      ssid: "",
      password: "",
    };
  }
  render() {
    return (
      <Page>
        <Navbar title="Setup" backLink="Back"/>
        <Block strong>
          <p>Before using this system you need to setup your Wifi credentials for your security system. Using network with password si highly recomended.
          Please enter your WiFi SSID and password in following rows and continue by clicking on 'Send'.</p>

          <p>Make sure your Bluetooth and Location service is turn on.</p>
        </Block>
        <List>
          <ListInput
            type="text"
            label="SSID:"
            required
            info="Name of your Wi-Fi AP"
            maxlength="32"
            placeholder="My Wi-Fi"
            value={this.state.ssid}
            onChange={(event) => this.setState({ssid: event.target.value})}
          />
          <ListInput
            type="password"
            label="Password:"
            maxlength="64"
            placeholder="My password"
            value={this.state.password}
            onChange={(event) => this.setState({password: event.target.value})}
          />
          <ListItem>
            <Button fill onClick={() => this.setupBLE(false)}>Send</Button>
          </ListItem>
        </List>

        <Block strong>
          <p>After setting your WiFi credentials you need to setup your application. This will set IP address of your security system in this aplication for other configurations.</p>
        </Block>

        <Block>
          <Button fill disabled={typeof localStorage.ip !== 'undefined'} onClick={() => this.setupBLE(true)}>Find main unit</Button>
        </Block>

        <Popup opened={this.state.popupOpened} onPopupClosed={() => this.setState({popupOpened : false})}>
          <Page>
            <Navbar title={this.state.popupTitle}>
              <NavRight>
                <Link popupClose>Close</Link>
              </NavRight>
            </Navbar>
            <Block>
              <p>{this.state.message}</p>
            </Block>
          </Page>
        </Popup>
      </Page>
    );
  }
  async setupBLE(read){
    var state = this;
    console.log(this);
    cordova.plugins.diagnostic.requestLocationAuthorization(status => {console.error(status)}, error => {console.error(error)});
    cordova.plugins.diagnostic.hasBluetoothLESupport(function(supported){
      console.log("Bluetooth LE is " + (supported ? "supported" : "unsupported"));
    }, error => {this.handle_error(error)});
    
    new Promise(function (resolve) {
      bluetoothle.initialize(resolve, { request: true, statusReceiver: false });
    }).then(initializeSuccess, error => {this.handle_error(error)});
    
    function initializeSuccess(result) {
      if (result.status === "enabled") {
        state.log("Bluetooth is enabled.");
        state.props.f7router.app.preloader.show();
        startScan();
      }
      else {
        state.handle_error({message: "Bluetooth is not enabled"});
        state.log(result);
      }
    }

    function startScan() {
      state.log("Starting scan for devices...", "status");
      setTimeout(function(){
        bluetoothle.startScan(startScanSuccess, error => {
          error(error);
          bluetoothle.stopScan(
            function() {
              state.log("end of scan not succ");
            }, error => {state.log(error)}
          );
        }, { services: [] });
      }, 3000);
    }

    function startScanSuccess(result) {
      state.log("startScanSuccess(" + result.status + ")");
      if (result.status === "scanStarted") {
          state.log("Scanning for devices (will continue to scan until you select a device)...", "status");
      }else if (result.status === "scanResult") {
        state.log(result)
        if (result.name == "ESP_main_unit"){
          bluetoothle.stopScan(
            function() {
              state.log("end of scan succ");
            }, error => {state.log(error)}
            );
            new Promise(function (resolve, reject) {
              bluetoothle.connect(resolve, reject, { address: result.address });
            }).then(connectSuccess(result.address), error => {
              state.log(error);
              state.log("138");
              bluetoothle.reconnect(succ => {connectSuccess(address)}, error => {state.log("139"); state.log(error); state.handle_error(error)}, { address: result.address });
            });
        }
      }
    }

    function connectSuccess(address) {
      new Promise(function (resolve, reject) {
        bluetoothle.discover(resolve, reject,
            { address: address });
      }).then(response => { 
        if (read){
          readService(address);
        }else{
          if(state.state.ssid){
            writeWifi(address);
          }else{
            state.handle_error("SSID should not be empty!");
          }
        }
        }, error => {error.message = error.message + ". Please try again.", state.handle_error(error)});
    }

    async function readService(address) {
      var serviceUuid = "1BA2ECFF-FFCE-4B4E-8562-78F5DCF950B3"
      await new Promise(function (resolve, reject) {
        bluetoothle.read(resolve, reject, { address: address, service: serviceUuid, characteristic: serviceUuid });
      }).then(succ => {
        state.setState({popupOpened : false})
        var arr = bluetoothle.encodedStringToBytes(succ.value);
        this.props.f7router.app.toast.show({
          text: 'Main unit was found and IP address was stored.',
          closeTimeout: 2000,
        })
        var ip = ""
        for (let element of arr) {
          ip = ip + '.' + element;
        }
        localStorage.ip = ip.substr(1);
        state.log(localStorage.ip)
        BleClose();
        state.props.f7router.app.views.main.router.navigate("/");

      }, error => state.handle_error(error));
    }

    var serviceUuid = "1BA2ECFF-FFCE-4B4E-8562-78F5DCF950B3"
    async function writeWifi(address) {
      var ssidCharUuid = "1BA2ECFF-FFCE-4B4E-8562-78F5DCF950B3"
      var bytes = bluetoothle.stringToBytes(state.state.ssid);
      var encodedString = bluetoothle.bytesToEncodedString(bytes);
      var params = {value: encodedString, service: serviceUuid,characteristic: ssidCharUuid, type:"noResponse", address: address}
      await new Promise(function (resolve, reject) {
        bluetoothle.write(resolve, reject, params);
      }).then(succ => {
        state.log(succ);
        writePass(address);
      }, error => state.handle_error(error));
    }

    async function writePass(address) {
      await new Promise(function (resolve, reject) {
        var passCharUuid = "83686E01-A69B-4BAF-94EC-4E1F3E674491"
        var bytes;
        if (state.state.password){
          bytes = bluetoothle.stringToBytes(state.state.password);
        }else{
          bytes = bluetoothle.stringToBytes("\0");
        }
        var encodedString = bluetoothle.bytesToEncodedString(bytes);
        var params = {value: encodedString, service: serviceUuid,characteristic: passCharUuid, type:"noResponse", address: address}
        bluetoothle.write(resolve, reject, params);
      }).then(succ => {
        state.log(succ);
        localStorage.removeItem("ip");
        state.setState({ popupTitle: "Success", popupOpened : true, password: "", ssid: "",
          message: "Wifi credentials was sucessfully stored. Wait for long beep and then press 'Find main unit'" })
      }, error => state.handle_error(error));
    }

    async function BleClose() {
      bluetoothle.close(success => {state.log(success)},error => {state.handle_error(error)},{ address: address })
      state.props.f7router.app.preloader.hide();
      state.props.f7router.app.views.main.router.navigate("/");
      return;
    }
  }
  log(msg) {
    if (typeof msg === "object") {
      msg = JSON.stringify(msg, null, "  ");
    }
    console.log(msg);
  }
  handle_error(error){
    this.log(error);
    this.props.f7router.app.preloader.hide();
    this.setState({ popupTitle: "Error", popupOpened : true, message: error.message })
  }
}