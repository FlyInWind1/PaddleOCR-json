"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const worker_threads_1 = require("worker_threads");
const path_1 = require("path");
const child_process_1 = require("child_process");
const currentPath = process.cwd();
const __default = {
    path: 'PaddleOCR_json.exe',
    args: ['--use_debug=0'],
    options: {
        argv0: undefined,
        stdio: 'pipe',
        detached: false,
        shell: false,
        windowsVerbatimArguments: undefined,
        windowsHide: true,
    },
    initTag: 'OCR init completed.',
};
function cargs(obj) {
    obj = Object.assign({}, obj);
    if (obj.image_dir === null)
        obj.image_dir = 'clipboard';
    else if (obj.image_dir)
        obj.image_dir = (0, path_1.resolve)(currentPath, obj.image_dir);
    if (obj.output !== undefined)
        obj.output = (0, path_1.resolve)(currentPath, obj.output);
    return obj;
}
function cout(data) {
    return {
        code: data.code,
        message: data.code - 100 ? data.data : '',
        data: data.code - 100 ? null : data.data,
    };
}
const end = 'at' in String ? (str) => str.at(-1) : (str) => str[str.length - 1];
if (!worker_threads_1.isMainThread) {
    new Promise((res) => {
        const { path = __default.path, args = [], options, } = worker_threads_1.workerData;
        const proc = (0, child_process_1.spawn)(path, args.concat(__default.args), {
            ...options,
            ...__default.options,
        });
        process.once('exit', proc.kill.bind(proc));
        proc.once('exit', process.exit);
        proc.stdout.on('data', function stdout(chunk) {
            if (!chunk.toString().match(__default.initTag))
                return;
            proc.stdout.off('data', stdout);
            return res(proc);
        });
    }).then((proc) => {
        process.stdout.write(`${proc.pid}\n`);
        process.stdout.clearLine;
        proc.stdout.pipe(process.stdout);
        proc.stderr.pipe(process.stderr);
        worker_threads_1.parentPort.on('message', (data) => {
            proc.stdin.write(`${JSON.stringify(cargs(data))}\n`);
        });
        const cache = [];
        proc.stdout.on('data', (chunk) => {
            const str = String(chunk);
            cache.push(str);
            if (end(str) !== '\n')
                return;
            worker_threads_1.parentPort.postMessage(cout(JSON.parse(cache.join(''))));
            cache.length = 0;
        });
    });
}
