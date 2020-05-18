import React from 'react';
import {
  Page,
  Navbar,
  List,
  Button,
  Card,
  CardFooter,
  CardHeader,
  Link,
  Popup,
  NavRight,
  Block,
  ListItem,
  BlockTitle,
  ListInput,
  CardContent,
} from 'framework7-react';
import axios from 'axios'

export default class extends React.Component {
  constructor(props) {
    super(props);

    this.state = {
      list: props.f7route.context.list,
      distance: props.f7route.context.distance,
      popupOpened: props.f7route.context.errorPopup,
      message: props.f7route.context.message,
      popupTitle: "Error",
    };
  }
  render() {
    const list = this.state.list;
    this.log(list);
    return (
      <Page>
        <Navbar title="Whitelist" backLink="Back" />
        <List>
          <ListInput
            label="Detection distance:"
            type="number"
            placeholder="N/A"
            info="Decimal number in meters"
            value={this.state.distance}
            onChange={(event) => this.setState({distance: event.target.value})}
          />
          <ListItem>
            <Button fill onClick={() => this.handleClick(this.state.distance, "RSSI")} text="Change"/>
          </ListItem>
        </List>
        <BlockTitle>Devices</BlockTitle>
        <List>
          {list.result.map((device, index) => (
            <Card key={index}>
              <CardHeader>{`Address: ${device.address}`}</CardHeader>
              {/* <CardContent>
                {`Last connection: ${device.last_connection  == 0 ? "N/A" : new Date(device.last_connection*1000).toLocaleString()}`}
              </CardContent> */}
              <CardFooter>
                <Link></Link>
                <Button fill raised color="red" onClick={() => this.handleClick(device.address.replace(/:/g,'', "remove"))}>Remove</Button>
              </CardFooter>
            </Card>
          ))}
        </List>
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
  handleClick (value, type) {
    if (typeof localStorage.ip == 'undefined'){
      this.setState({ popupTitle: "Error", popupOpened : true, message: "Please click on 'Find main unit' button on Setup page" })
      return;
    }
    var url = 'http://' + localStorage.ip;
    var data;
    if(type == "RSSI"){
      url = url + '/ble/device/rssi'
      data = {
        distance: value,
      }
    }else{
      url = url + '/ble/device/remove'
      data = {
        address: value,
      }
    }
    axios({
      method: 'post',
      url: url,
      timeout: 3000,
      data: data,
    }).then(response => {this.setState({ popupTitle: "Sucess", popupOpened : true, message: response.data })}, 
    error => {
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
}