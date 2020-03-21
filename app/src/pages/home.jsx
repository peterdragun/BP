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
} from 'framework7-react';

export default () => (
  <Page name="home">
    {/* Top Navbar */}
    <Navbar sliding={false} large>
      <NavLeft>
        <Link iconIos="f7:menu" iconAurora="f7:menu" iconMd="material:menu" panelOpen="left" />
      </NavLeft>
      <NavTitle sliding>Home security</NavTitle>
      <NavTitleLarge>Home security</NavTitleLarge>
    </Navbar>

    <BlockTitle>Navigation</BlockTitle>
    <List>
      <ListItem link="/scan/" title="Scan"/>
      <ListItem link="/whitelist/" title="Whitelist"/>
      <ListItem link="/about/" title="About"/>
    </List>

    <List>
      <ListItem
        title="Dynamic (Component) Route"
        link="/dynamic-route/blog/45/post/125/?foo=bar#about"
      />
      <ListItem
        title="Default Route (404)"
        link="/load-something-that-doesnt-exist/"
      />
    </List>

  </Page>
);