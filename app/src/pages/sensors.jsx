import React from 'react';
import {
  Page,
  Navbar,
  List,
  Button,
  Card,
  CardContent,
  CardFooter,
  CardHeader,
  Link,
  Popup,
  NavRight,
  Block,
  BlockTitle,
} from 'framework7-react';
import axios from 'axios'

export default class extends React.Component {
  constructor(props) {
    super(props);

    this.state = {
      sensors: props.f7route.context.sensors,
      unknown: props.f7route.context.unknown,
      popupTitle: "Error",
      popupOpened: props.f7route.context.errorPopup,
      message: props.f7route.context.message,
    };
  }
  render() {
    const sensors = this.state.sensors;
    const unknown = this.state.unknown;
    var newSensor = ""
    if (unknown) {
      newSensor = (
        <List>
          <BlockTitle>New sensor</BlockTitle>
          <Card>
            <CardHeader>{`Address: ${this.state.unknown.address}`}</CardHeader>
            <CardContent>
              <p>{`Type: ${this.state.unknown.type == 0 ? "PIR" : "Magnetic sensor"}`}</p>
            </CardContent>
            <CardFooter>
              <Link></Link>
              <Button fill raised onClick={() => this.handleClickAdd(unknown.address.replace(/:/g,''))}>Add</Button>
            </CardFooter>
          </Card>
        </List>
      )
    }
    return (
      <Page ptr onPtrRefresh={this.reload.bind(this)}>
        <Navbar title="Sensors" backLink="Back" />
        <List>
          {sensors.result.map((device, index) => (
            <Card key={index}>
              <CardHeader>{`Address: ${device.address}`}</CardHeader>
              <CardContent>
                <p>{`Type: ${device.type == 0 ? "PIR" : "Magnetic sensor"}`}</p>
                <p>{`Last alarm: ${device.last_alarm == 0 ? "N/A" : new Date(device.last_alarm*1000).toLocaleString()}`}</p>
                <p>{`Last connection: ${device.last_connection  == 0 ? "N/A" : new Date(device.last_connection*1000).toLocaleString()}`}</p>
              </CardContent>
              <CardFooter>
                <Link></Link>
                <Button fill raised color="red" onClick={() => this.handleClickRemove(device.address.replace(/:/g,''), index)}>Remove</Button>
              </CardFooter>
            </Card>
          ))}
        </List>
        {newSensor}
        
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
  handleClickRemove (address, index) {
    if (typeof localStorage.ip == 'undefined'){
      this.setState({ popupTitle: "Error", popupOpened : true, message: "Please click on 'Find main unit' button on Setup page" })
      return;
    }
    axios({
      method: 'post',
      url: 'http://' + localStorage.ip + '/sensors/remove',
      timeout: 3000,
      data: {
        address: address,
      }
    }).then(
      response => {
        sensors = this.state.sensors;
        sensors.splice(index, 1);
        this.setState({ popupTitle: "Success", popupOpened : true, message: response.data, sensors: sensors})
      },
    error => {
      console.log(error);
      var message = "Timeout"
      if (error.response){
        message = error.response.data
      }
      this.setState({ popupTitle: "Error", popupOpened : true, message: message })
    });
  }
  handleClickAdd (address) {
    if (typeof localStorage.ip == 'undefined'){
      this.setState({ popupTitle: "Error", popupOpened : true, message: "Please click on 'Find main unit' button on Setup page" })
      return;
    }
    this.log(address);
    axios({
      method: 'post',
      url: 'http://' + localStorage.ip + '/sensors/add',
      timeout: 3000,
      data: {
        address: address,
      }
    }).then(response => {this.log(response); this.setState({ popupTitle: "Success", popupOpened : true, message: response.data })},
    error => {
      this.log(error);
      console.log(error);
      var message = "Timeout"
      if (error.response){
        message = error.response.data
      }
      this.setState({ popupTitle: "Error", popupOpened : true, message: message })
    });
  }
  log(msg) {
    if (typeof msg === "object") {
      msg = JSON.stringify(msg, null, "  ");
    }
    console.log(msg);
  }
  reload(done){
    if (typeof localStorage.ip == 'undefined'){
      this.setState({ popupTitle: "Error", popupOpened : true, message: "Please click on 'Find main unit' button on Setup page" })
      return;
    }
    var {sensors} = this.state;
    axios({
      method: 'get',
      url: 'http://' + localStorage.ip + '/sensors/list',
      timeout: 3000
    }).then(response => {
      this.log(response);
      this.setState({sensors: {result: response.data.list}, unknown: response.data.unknown});
      done();
    }, error => {
      this.log(error);
      console.log(error);
      var message = "Timeout"
      if (error.response){
        message = error.response.data
      }
      this.setState({ popupTitle: "Error", popupOpened : true, message: message })
      done();
    });
  }
}