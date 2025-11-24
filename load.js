import http from 'k6/http';
import { check } from 'k6';

export let options = {
    vus: __ENV.VUS,
    duration: '20s',
};

export default function () {
    let r = Math.random();

    if (r > 0.5) {
        let id = Math.floor(Math.random() * 999999);
        let res = http.get(`http://localhost:8080/Read?id=${id}`);
        check(res, {
            'status ok/404': (r) => r.status === 200 || r.status === 404,
        });
    } else {
        let id = Math.floor(Math.random() * 1_000_000);
        let res = http.post(`http://localhost:8080/Create?id=${id}`, "kjgakadn");
        check(res, {
            'set ok': (r) => r.status === 200,
        });
    }
}