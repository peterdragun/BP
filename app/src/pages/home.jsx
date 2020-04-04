import React from 'react';
import {
  Page,
  Navbar,
  NavLeft,
  NavTitle,
  NavTitleLarge,
  Link,
  BlockTitle,
  List,
  ListItem,
  Block,
  Button,
  Popup,
  NavRight
} from 'framework7-react';
import axios from 'axios'

export default class Home extends React.Component {
  constructor(props) {
    super(props);

    this.state = {
      popupOpened: false,
      errorPopupOpened: false,
      message: "",
    }
  }
  render() {
    return (
      <Page name="home">
        {/* Top Navbar */}
        <Navbar sliding={false} large>
          <NavLeft>
            <Link iconIos="f7:menu" iconAurora="f7:menu" iconMd="material:menu" panelOpen="left" />
          </NavLeft>
          <NavTitle sliding>Home security</NavTitle>
          <NavTitleLarge>Home security</NavTitleLarge>
        </Navbar>

        <Block>
          <Button fill onClick={() => this.handleClick()}>Activate</Button>
        </Block>

        <BlockTitle>Navigation</BlockTitle>
        <List>
          <ListItem link="/scan/" title="Scan"/>
          <ListItem link="/whitelist/" title="Whitelist"/>
          <ListItem link="/change-code/" title="Change alarm code"/>
          <ListItem link="/about/" title="About"/>
        </List>

        <Popup opened={this.state.succPopupOpened} onPopupClosed={() => this.setState({succPopupOpened : false})}>
          <Page>
            <Navbar title="Success">
              <NavRight>
                <Link popupClose>Close</Link>
              </NavRight>
            </Navbar>
            <Block>
              <p>{this.state.message}</p>
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
              <p>{this.state.message}</p>
            </Block>
          </Page>
        </Popup>
      </Page>
    );
  }
  handleClick () {
    axios({
      method: 'get',
      // url: 'http://esp-home.local/system/arm', // wont work on android, pls google
      url: 'http://192.168.1.45/system/arm',
      timeout: 3000
    }).then(response => {this.setState({ succPopupOpened : true, message: response.data })}, 
    error => {
      console.log(error);
      var message = "Timeout"
      if (error.response){
        message = error.response.data
      }
      this.setState({ errorPopupOpened : true, message: message })
    });
  }
}