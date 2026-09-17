import copy
import importlib.util
import json
import unittest
from pathlib import Path

spec=importlib.util.spec_from_file_location('status_icon_gen',Path(__file__).resolve().parents[1]/'status_icon_gen.py')
gen=importlib.util.module_from_spec(spec)
spec.loader.exec_module(gen)

class StatusGeneratorTests(unittest.TestCase):
    def setUp(self):
        self.doc=json.loads(gen.SOURCE.read_text(encoding='utf-8'))
    def test_polarity_padding_roundtrip(self):
        rows=['#.......#','.#.......']
        self.assertEqual(gen.encode(rows),bytes([1,1,2,0]))
        self.assertEqual(gen.decode(gen.encode(rows),9,2),rows)
    def test_current_header(self):
        self.assertEqual(gen.generate(self.doc),gen.HEADER.read_text(encoding='utf-8'))
    def test_add_delete_reorder_and_hidden(self):
        self.doc['bitmaps'].append({'name':'new_icon','rows':['#..','..#']})
        self.doc['layout'].insert(0,{'id':'new_slot','bitmap':'new_icon','visible':False})
        text=gen.generate(self.doc)
        self.assertIn('status_data_new_icon, 3, 2,',text)
        self.assertLess(text.index('{"new_slot"'),text.index('{"battery"'))
        self.doc['layout']=[]
        self.assertIn('statusIconEntryCount = 0',gen.generate(self.doc))
    def test_reject_bad_sources(self):
        for rows in ([],[''],['##','#'],['X'],['.'*256]):
            doc=copy.deepcopy(self.doc); doc['bitmaps'][0]['rows']=rows
            with self.assertRaises(ValueError): gen.generate(doc)
        doc=copy.deepcopy(self.doc); doc['layout'][0]['bitmap']='missing'
        with self.assertRaises(ValueError): gen.generate(doc)
        doc=copy.deepcopy(self.doc); doc['layout'].append(doc['layout'][0])
        with self.assertRaises(ValueError): gen.generate(doc)

if __name__=='__main__': unittest.main()
