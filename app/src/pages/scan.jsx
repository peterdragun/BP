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
} from 'framework7-react';
import axios from 'axios'

export default class extends React.Component {
  constructor(props) {
    super(props);

    this.state = {
      scan: props.f7route.context.scan,
      popupTitle: "Error",
      popupOpened: props.f7route.context.errorPopup,
      message: props.f7route.context.message,
    };
  }
  render() {
    const scan = this.state.scan;
    return (
      <Page ptr onPtrRefresh={this.reload.bind(this)}>
        <Navbar title="Scan" backLink="Back" />
        <List>
          {scan.result.map((device, index) => (
            <Card key={index}>
              <CardHeader>{`Address: ${device.address}`}</CardHeader>
              <CardContent>{`Name: ${device.name}`}</CardContent>
              <CardFooter>
                <Link></Link>
                <Button fill raised onClick={() => this.handleClick(device.address.replace(/:/g,''))}>Add</Button>
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
  handleClick (address) {
    if (typeof localStorage.ip == 'undefined'){
      this.setState({ popupTitle: "Error", popupOpened : true, message: "Please click on 'Find main unit' button on Setup page" })
      return;
    }
    axios({
      method: 'post',
      url: 'http://' + localStorage.ip + '/ble/device/add',
      timeout: 6000,
      data: {
        address: address,
      }
    }).then(response => {this.setState({ popupTitle: "Success", popupOpened : true, message: response.data })},
    error => {
      console.log(error);
      var message = "Timeout"
      if (error.response){
        message = error.response.data
      }
      this.setState({ popupTitle: "Error", popupOpened : true, message: message })
    });
  }
  reload(done){
    if (typeof localStorage.ip == 'undefined'){
      this.setState({ popupTitle: "Error", popupOpened : true, message: "Please click on 'Find main unit' button on Setup page" })
      return;
    }
    var {scan} = this.state;
    axios({
      method: 'get',
      url: 'http://' + localStorage.ip + '/ble/scan',
      timeout: 8000
    }).then(response => {
      console.log(response);
      scan = {result: this.removeDuplicates(response.data)};
      this.setState({scan});
      done();
    }, error => {
      console.log(error);
      var message = "Timeout"
      if (error.response){
        message = error.response.data
      }
      this.setState({ popupTitle: "Success", popupOpened : true, message: message })
      done();
    });
  }
  removeDuplicates(array){
    var trimmedArray = [];
    var values = [];
    var value;

    for(var i = 0; i < array.length; i++) {
      value = array[i]["address"];
      if(values.indexOf(value) === -1) {
        trimmedArray.push(array[i]);
        values.push(value);
      }else if (array[i]["name"]){
        trimmedArray[values.indexOf(value)]["name"] = array[i]["name"];
      }
    }
    return trimmedArray;
  }
}