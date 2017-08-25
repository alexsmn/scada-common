angular.module('scada').factory('Session', ['$q', '$rootScope', function($q, $rootScope) {
  var Session = {};

  var Proto = dcodeIO.ProtoBuf.loadProtoFile('scada.proto').build('protocol');
  Session.ConstantNodeId = Proto.ConstantNodeId;

  // Monitored Item Statuses.
  Session.CONNECTING = 0;
  Session.CONNECTED = 1;
  Session.ERROR = 2;

  // Monitored Item Statuses.
  Session.CONNECTING = 0;
  Session.CONNECTED = 1;
  Session.ERROR = 2;

  var ws = null;

  var nextRequestId = 0;
  var requests = [];
  var subscriptionId = null;
  var monitoredItems = [];
  var connectDefer = $q.defer();
  var disconnectDefer = $q.defer();

  Session.onDisconnect = disconnectDefer.promise;

  var nodes = {};

  function onConnectionClosed() {
    ws = null;

    r = requests;
    requests = {};
    var response = {
      status: false
    };
    for (var i = 0; i < r.length; ++i) {
      response.requestId = r[i].requestId;
      r[i].callback.resolve(response);
    }

    disconnectDefer.resolve();
    connectDefer.reject();
  }

  function onMessageReceived(message) {
    for (var i = 0; i < message.responses.length; ++i) {
      var response = message.responses[i];
      var request = popRequest(response.request_id);
      if (request) {
        if (response.status.code == 0)
          request.callback.resolve(response);
        else
          request.callback.reject(response.status.code);
        $rootScope.$apply();
      }
    }

    for (var i = 0; i < message.notifications.length; ++i) {
      var notification = message.notifications[i];
      for (var j = 0; j < notification.data_changes.length; ++j)
        dataChangeNotification(notification.data_changes[j]);
    }
  }

  function dataChangeNotification(dataChange) {
    var monitoredItem = findMonitoredItem(dataChange.monitored_item_id);
    if (!monitoredItem)
      return;
    monitoredItem.value = dataChange.data_value.value;
    monitoredItemChanged(monitoredItem);
  }

  function nodeIdToProto(nodeId) {
    var p = new Proto.NodeId;
    p.number = nodeId;
    return p;
  }

  function nodeIdToString(nodeId) {
    return nodeId.toString();
  }

  function valueFromProto(protoValue) {
    if (protoValue.bool_value != null)
      return protoValue.bool_value;
    else if (protoValue.int32_value != null)
      return protoValue.int32_value;
    else if (protoValue.int64_value != null)
      return protoValue.int64_value;
    else if (protoValue.double_value != null)
      return protoValue.double_value;
    else if (protoValue.string_value != null)
      return protoValue.string_value;
    else
      return null;
  }

  function getPropName(id) {
    if (id == 3)
      return 'name';
    else if (id == 39)
      return 'format';
    else
      return null;
  }

  function createNode(nodeId, typeId, parentId) {
    var node = {
      id: nodeId,
      typeId: typeId,
      children: []
    };

    if (parentId) {
      node.parent = Session.getNode(parentId);
      if (!node.parent)
        throw "Wrong parent " + nodeIdToString(parentId);
      node.parent.children.push(node);
    }

    nodes[nodeId] = node;

    return node;
  }

  function loadNodes(nodes) {
    for (var i = 0; i < nodes.length; ++i) {
      var data = nodes[i];
      var node = createNode(data.node_id.number,
                            data.type_node_id.number,
                            data.parent_node_id.number);
      for (var j = 0; j < data.properties.length; ++j) {
        var prop = data.properties[j];
        var propName = getPropName(prop.id);
        if (propName)
          node[propName] = valueFromProto(prop.value);
      }
    }
  }

  function monitoredItemChanged(monitoredItem) {
    for (var i = 0; i < monitoredItem.observers.length; ++i)
      monitoredItem.observers[i]();
  }

  function popRequest(requestId) {
    for (var i = 0; i < requests.length; ++i) {
      var request = requests[i];
      if (request.request_id == requestId) {
        requests.splice(i, 1);
        return request;
      }
    }
    return null;
  }

  function sendRequest(request) {
    var defer = $q.defer();
    var requestId = nextRequestId++;
    request.request_id = requestId;

    var message = new Proto.Message();
    message.requests.push(request);
    console.log('Message OUT ', message);
    ws.send(message.toBuffer());

    request.callback = defer;
    requests.push(request);
    return defer.promise;
  };

  function createSubscription() {
    var create_subscription = new Proto.CreateSubscription();

    var request = new Proto.Request();
    request.create_subscription = create_subscription;

    var defer = $q.defer();
    sendRequest(request).then(function(response) {
      defer.resolve(response.create_subscription_result.subscription_id);
    }, function(error) {
      defer.reject(error);
    });
    return defer.promise;
  }

  function findMonitoredItem(monitoredItemId) {
    for (var i = 0; i < monitoredItems.length; ++i) {
      var item = monitoredItems[i];
      if (item.id == monitoredItemId)
        return item;
    }
    return null;
  }

  Session.getNode = function(nodeId) {
    return nodes[nodeId];
  }

  Session.connect = function (url) {
    ws = new WebSocket(url);
    ws.binaryType = "arraybuffer";
    ws.onopen = function () {
      console.log('Socket opened');
      connectDefer.resolve();
    };
    ws.onclose = function () {
      console.log('Socket closed');
      onConnectionClosed();
    };
    ws.onmessage = function (event) {
      var message = Proto.Message.decode(event.data);
      console.log('Message IN ', message);
      onMessageReceived(message);
    };

    return connectDefer.promise;
  }

  Session.createSession = function(userName, password) {
    var create_session = new Proto.CreateSession();
    create_session.user_name = userName;
    create_session.password = password ? password : '';
    create_session.delete_existing = true;
    create_session.protocol_version_major = Proto.ProtocolVersionMajor.PROTOCOL_VERSION_MAJOR;
    create_session.protocol_version_minor = Proto.ProtocolVersionMinor.PROTOCOL_VERSION_MINOR;

    var request = new Proto.Request();
    request.create_session = create_session;

    var defer = $q.defer();
    sendRequest(request).then(function(response) {
      createSubscription(request).then(function(id) {
        subscriptionId = id;
        defer.resolve();
      }, function(error) {
        defer.reject(error);
      });
    }, function(error) {
      defer.reject(error);
    });
    return defer.promise;
  };

  Session.createView = function(parentNodeId) {
    var create_view = new Proto.CreateView();
    create_view.parent_node_id = nodeIdToProto(parentNodeId);

    var request = new Proto.Request();
    request.create_view = create_view;

    var defer = $q.defer();
    sendRequest(request).then(function(response) {
      if (response.create_view_result.nodes)
        loadNodes(response.create_view_result.nodes);
      defer.resolve();
    }, function(error) {
      defer.reject(error);
    });
    return defer.promise;
  };

  Session.getMonitoredItem = function(nodeId) {
    for (var i = 0; i < monitoredItems.length; ++i) {
      var item = monitoredItems[i];
      if (item.nodeId.first == nodeId.first &&
          item.nodeId.second == nodeId.second)
        return item;
    }

    var monitoredItem = {
      nodeId: nodeId,
      status: Session.CONNECTING,
      observers: []
    };

    var create_monitored_item = new Proto.CreateMonitoredItem();
    create_monitored_item.subscription_id = subscriptionId;
    create_monitored_item.path = '{' + nodeId.first + '.' + nodeId.second + '}';
    create_monitored_item.attribute_id = Proto.AttributeId.DATA_VALUE;

    var request = new Proto.Request();
    request.create_monitored_item = create_monitored_item;

    sendRequest(request).then(function(response) {
      monitoredItem.id = response.create_monitored_item_result.monitored_item_id;
      monitoredItem.status = Session.CONNECTED;
      monitoredItemChanged(monitoredItem);
    }, function(error) {
      monitoredItem.status = Session.ERROR;
      monitoredItemChanged(monitoredItem);
    });

    monitoredItems.push(monitoredItem);
    return monitoredItem;
  };

  return Session;
}]);