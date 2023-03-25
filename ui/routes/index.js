var express = require('express');
var router = express.Router();

const addon = require('../build/Release/addon');
const {
  Worker, isMainThread, parentPort, workerData,
} = require('node:worker_threads');


/* GET home page. */
router.get('/', function(req, res, next) {
  res.render('index', { title: 'Express' });
});


var worker = null;

router.post('/chess-api', async function(req, res, next) {
  var req_json = req.body;

  function setImmediatePromise() {
    return new Promise((resolve, reject) => {
        // do we need setImmediate here?
      setImmediate(() => {
        if (worker != null) {
          worker.postMessage('cancel');
          worker.terminate();
        }
        worker = new Worker('./routes/eval_board.js', {
          workerData: req_json,
        });
        worker.on('message', resolve);
        worker.on('error', reject);
        worker.on('exit', (code) => {
          resolve(null);
//          if (code !== 0)
//            reject(new Error(`Worker stopped with exit code ${code}`));
        });

      });
    });
  }

  var move_result = await setImmediatePromise();

  if (move_result != null) {
    res.json(move_result);
  } else {
    res.json({});
  }
})

module.exports = router;

