import React from 'react';
import { Device }  from 'framework7/framework7-lite.esm.bundle.js';
import {
  App,
  Panel,
  View,
  Page,
  Navbar,
  BlockTitle,
  List,
  ListItem,
} from 'framework7-react';

import cordovaApp from '../js/cordova-app';
import routes from '../js/routes';

export default class extends React.Component {
  constructor() {
    super();

    this.state = {
      // Framework7 Parameters
      f7params: {
        id: 'io.framework7.myapp', // App bundle ID
        name: 'Home security', // App name
        theme: 'auto', // Automatic theme detection

        // App routes
        routes: routes,

        // Input settings
        input: {
          scrollIntoViewOnFocus: Device.cordova && !Device.electron,
          scrollIntoViewCentered: Device.cordova && !Device.electron,
        },
        // Cordova Statusbar settings
        statusbar: {
          iosOverlaysWebView: true,
          androidOverlaysWebView: false,
        },
      },
    }
  }
  render() {
    return (
      <App params={ this.state.f7params } themeDark>

        {/* Left panel with cover effect when hidden */}
        <Panel left cover themeDark visibleBreakpoint={960}>
          <View>
            <Page>
              <Navbar title="Menu"/>
              <BlockTitle>Control Main View</BlockTitle>
              <List>
                <ListItem link="/scan/" view=".view-main" panelClose title="Scan"/>
                <ListItem link="/whitelist/" view=".view-main" panelClose title="Whitelist"/>
              </List>
              <BlockTitle>About application</BlockTitle>
              <List>
                <ListItem link="/about/" view=".view-main" panelClose title="About"/>
              </List>
            </Page>
          </View>
        </Panel>

        {/* Your main view, should have "view-main" class */}
        <View main className="safe-areas" url="/" />

      </App>
    )
  }
  componentDidMount() {
    this.$f7ready((f7) => {
      // Init cordova APIs (see cordova-app.js)
      if (Device.cordova) {
        cordovaApp.init(f7);
      }
      // Call F7 APIs here
    });
  }
}