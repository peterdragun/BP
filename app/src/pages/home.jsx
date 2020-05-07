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
  NavRight,
} from 'framework7-react';
import axios from 'axios'

export default class Home extends React.Component {
  constructor(props) {
    super(props);

    this.state = {
      popupOpened: props.f7route.errorPopup,
      message: "",
      popupTitle: "Error",
      status: props.f7route.context.status,
      alarm: props.f7route.context.alarm,
      sensors: props.f7route.context.sensors,
      notResponding: props.f7route.context.notResponding,
      refresh: false,
      lastRefresh: "N/A",
    }
  }
  render() {
    return (
      <Page name="home" ptr onPtrRefresh={this.reload.bind(this)}>
        <Navbar sliding={false} large>
          <NavLeft>
            <Link iconIos="f7:menu" iconAurora="f7:menu" iconMd="material:menu" panelOpen="left" />
          </NavLeft>
          <NavTitle sliding>Home security</NavTitle>
          <NavTitleLarge>Home security</NavTitleLarge>
        </Navbar>

        <List>
          <ListItem header="Current status" title={this.state.status}/>
          <ListItem header="Connected sensors" title={this.state.sensors}/>
          <ListItem header="Sensors not responding" title={this.state.notResponding}/>
          <ListItem header="Last alarm" title={this.state.alarm}/>
          <ListItem header="IP address" title={localStorage.ip}/>
          <ListItem>
            <Button disabled={this.state.refresh} fill onClick={() => this.loadStatus()}>Refresh</Button>
            <p>{`Last refresh ${this.state.lastRefresh}`}</p>
          </ListItem>
        </List>

        <Block>
          <Button fill onClick={() => this.handleClick()}>Activate</Button>
        </Block>

        <BlockTitle>Navigation</BlockTitle>
        <List>
          <ListItem link="/scan/" title="Scan"/>
          <ListItem link="/whitelist/" title="Whitelist"/>
          <ListItem link="/sensors/" panelClose title="Sensors"/>
          <ListItem link="/change-code/" title="Change alarm code"/>
          <ListItem link="/setup/" panelClose title="Setup"/>
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
  reload(done){
    this.loadStatus();
    done();
  }
  loadStatus(){
    if (typeof localStorage.ip == 'undefined'){
      this.setState({ popupTitle: "Error", popupOpened : true, message: "Please click on 'Find main unit' button on Setup page" })
      return;
    }
    this.setState({refresh: true});
    axios({
      method: 'get',
      url: 'http://' + localStorage.ip + '/status',
      timeout: 3000
    }).then(response => {
      response = response.data;
      console.log(response.alarm)
      var alarm;
      if (response.alarm == 0){
        alarm = "N/A"
      }else{
        alarm = new Date(response.alarm*1000).toLocaleString();
      }
      this.setState({status: response.status, alarm: alarm, sensors: response.sensors, notResponding: response.notResponding,
        refresh: false, lastRefresh: new Date().toLocaleString()})
    },
    error => {
      console.error(error);
      var message = "Timeout"
      if (error.response){
        message = error.response.data
      }
      this.setState({ popupTitle: "Error", popupOpened : true, message: message })
    });
  }
  handleClick () {
    if (typeof localStorage.ip == 'undefined'){
      this.setState({ popupTitle: "Error", popupOpened : true, message: "Please click on 'Find main unit' button on Setup page" })
      return;
    }
    axios({
      method: 'get',
      url: 'http://' + localStorage.ip + '/system/arm',
      timeout: 3000
    }).then(response => {this.setState({ popupTitle: "Success", popupOpened : true, message: response.data })}, 
    error => {
      console.error(error);
      var message = "Timeout"
      if (error.response){
        message = error.response.data
      }
      this.setState({ popupTitle: "Error", popupOpened : true, message: message })
    });
  }
}