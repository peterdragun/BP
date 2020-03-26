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
      succPopupOpened: false,
      errorPopupOpened: false,
    };
  }
  render() {
    const scan = this.state.scan;
    return (
      <Page>
        <Navbar title="Scan" backLink="Back" />
        <List>
          {scan.result.map((device, index) => (
            <Card key={index}>
              <CardHeader>{`Address: ${device.address}`}</CardHeader>
              <CardContent>{`Name: ${device.name}`}</CardContent>
              <CardFooter>
                <Link></Link>
                <Button fill raised onClick={() => this.handleClick(device.address)}>Add</Button>
              </CardFooter>
            </Card>
          ))}
        </List>
        <Popup opened={this.state.succPopupOpened} onPopupClosed={() => this.setState({succPopupOpened : false})}>
          <Page>
            <Navbar title="Success">
              <NavRight>
                <Link popupClose>Close</Link>
              </NavRight>
            </Navbar>
            <Block>
              <p>Device was successfully added to whitelist.</p>
            </Block>
          </Page>
        </Popup>
        <Popup opened={this.state.errorPopupOpened} onPopupClosed={() => this.setState({errorPopupOpened : false})}>
          <Page>
            <Navbar title="Error">
              <NavRight>
                <Link popupClose>Close</Link>
              </NavRight>
            </Navbar>
            <Block>
              <p>Error ocured.</p>
            </Block>
          </Page>
        </Popup>
      </Page>
    );
  }
  handleClick (address) {
    axios({
      method: 'post',
      url: 'http://192.168.1.45/ble/address/add',
      data: {
        address: address,
      }
    }).then(response => {this.setState({ succPopupOpened : true })}, error => { console.log(error), this.setState({ errorPopupOpened : true })} );
  }
}