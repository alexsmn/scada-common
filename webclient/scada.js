angular.module('scada', ['ngRoute', 'ui.bootstrap', 'treeControl'])

.config(function($routeProvider) {
  $routeProvider
    .when('/', {
      templateUrl: 'login.html'
    })
    .when('/main', {
      templateUrl: 'main.html'
    })
    .otherwise({
      redirectTo: '/'
    });
})

.controller('LoginController', ['$scope', '$location', 'Session', function($scope, $location, Session) {
  $scope.userName = 'root';
  $scope.working = false;
  $scope.errorMessage = null;

  $scope.login = function (userName, password) {
    $scope.errorMessage = null;
    $scope.working = true;

    Session.connect('ws://localhost:8082').then(function () {
      Session.createSession(userName, password).then(function () {
        Session.createView(Session.ConstantNodeId.ROOT).then(function () {
          console.log('Session created');
          $location.path('/main');
        }, function () {
          error("Internal error");
        });
      }, function () {
        error("Login error");
      });
    }, function () {
      error("Connection error");
    });

    function error(message) {
      $scope.errorMessage = message;
      $scope.working = false;
    }
  }
}])

.controller('TreeController', ['$scope', 'Session', function($scope, Session) {
  $scope.node = Session.getNode(Session.ConstantNodeId.OBJECTS);
}])

.controller('ObjectTreeController', ['$scope', 'Session', function ($scope, Session) {
  $scope.objectTreeModel = Session.getNode(Session.ConstantNodeId.OBJECTS);
  $scope.add = function ($event, node) {
    alert(node.name);
    $event.stopPropagation();
  };
}])

.controller('TableController', ['$scope', '$modal', 'Session', function($scope, $modal, Session) {
  $scope.items = [];

  function formatValue(value, format) {
    if (!format)
      return value;
    var dot = format.indexOf('.');
    var precision = dot == -1 ? 0 : format.length - dot;
    return value.toFixed(precision);
  }

  function updateItem(item) {
    item.title = item.node ? item.node.name : '';
    item.value = !item.monitoredItem ? null :
      item.monitoredItem.status == Session.CONNECTING ? "Connecting" :
      item.monitoredItem.status == Session.ERROR ? "Error" :
      formatValue(item.monitoredItem.value, item.node ? item.node.format : null);
    $scope.$apply();
  }

  function addItem(node) {
    var monitoredItem = Session.getMonitoredItem(node.id);
    var item = { node: node, monitoredItem: monitoredItem };
    item.updater = function() { updateItem(item); }
    monitoredItem.observers.push(item.updater);
    $scope.items.push(item);
    updateItem(item);
  }

  $scope.add = function() {
    $modal.open({
      templateUrl: 'select-node.html',
      controller: 'SelectNodeController'
    }).result.then(function(nodes) {
      for (var i = 0; i < nodes.length; ++i)
        addItem(nodes[i]);
    });
  };
}])

.controller('SelectNodeController', function($scope, $modalInstance, Session) {
  $scope.selection = [];

  $scope.isSelected = function(node) {
    return $scope.selection.indexOf(node) != -1;
  }

  $scope.toggleSelection = function(node) {
    var i = $scope.selection.indexOf(node);
    if (i == -1)
      $scope.selection.push(node);
    else
      $scope.selection.splice(i, 1);
  }

  $scope.ok = function() {
    $modalInstance.close($scope.selection);
  }

  $scope.cancel = function() {
    $modalInstance.dismiss('cancel');
  }
})

.run(function ($rootScope, $route, $location, Session) {
  Session.onDisconnect.then(function() {
    $location.path('/');
  });
  /*$rootScope.$on('$routeChangeStart', function(event, next) {
    if ($location.path() != '/' && !Session.created())
      $location.path('/');
  });*/
});