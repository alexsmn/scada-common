Opc.Ua.ModelCompiler.exe -d2 model.xml -c node_ids.csv -o generated

python compile_node_ids.py > scada_node_ids.h
